/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

/** @file  MD5Parser.cpp
 *  @brief Implementation of the MD5 parser class
 */
#include "AssimpPCH.h"

// internal headers
#include "MD5Loader.h"
#include "MaterialSystem.h"
#include "fast_atof.h"
#include "ParsingUtils.h"
#include "StringComparison.h"

#if defined(__QNXNTO__)
#include <stdio.h>
#endif

using namespace Assimp;
using namespace Assimp::MD5;

// ------------------------------------------------------------------------------------------------
// Parse the segment structure fo a MD5 file
MD5Parser::MD5Parser(char* _buffer, unsigned int _fileSize )
{
    ai_assert(NULL != _buffer && 0 != _fileSize);

    buffer = _buffer;
    fileSize = _fileSize;
    lineNumber = 0;

    DefaultLogger::get()->debug("MD5Parser begin");

    // parse the file header
    ParseHeader();

    // and read all sections until we're finished
    bool running = true;
    while (running)    {
        mSections.push_back(Section());
        Section& sec = mSections.back();
        if (!ParseSection(sec))    {
            break;
        }
    }

    if ( !DefaultLogger::isNullLogger())    {
        char szBuffer[128]; // should be sufficiently large
        ::sprintf(szBuffer,"MD5Parser end. Parsed %i sections",(int)mSections.size());
        DefaultLogger::get()->debug(szBuffer);
    }
}

// ------------------------------------------------------------------------------------------------
// Report error to the log stream
/*static*/ void MD5Parser::ReportError (const char* error, unsigned int line)
{
    char szBuffer[1024];
    ::sprintf(szBuffer,"[MD5] Line %i: %s",line,error);
    throw DeadlyImportError(szBuffer);
}

// ------------------------------------------------------------------------------------------------
// Report warning to the log stream
/*static*/ void MD5Parser::ReportWarning (const char* warn, unsigned int line)
{
    char szBuffer[1024];
    ::sprintf(szBuffer,"[MD5] Line %i: %s",line,warn);
    DefaultLogger::get()->warn(szBuffer);
}

// ------------------------------------------------------------------------------------------------
// Parse and validate the MD5 header
void MD5Parser::ParseHeader()
{
    // parse and validate the file version
    SkipSpaces();
    if (!TokenMatch(buffer,"MD5Version",10))    {
        ReportError("Invalid MD5 file: MD5Version tag has not been found");
    }
    SkipSpaces();
    unsigned int iVer = ::strtol10(buffer,(const char**)&buffer);
    if (10 != iVer)    {
        ReportError("MD5 version tag is unknown (10 is expected)");
    }
    SkipLine();

    // print the command line options to the console
    // FIX: can break the log length limit, so we need to be careful
    char* sz = buffer;
    while (!IsLineEnd( *buffer++)) {}
    DefaultLogger::get()->info(std::string(sz,std::min((uintptr_t)MAX_LOG_MESSAGE_LENGTH, (uintptr_t)(buffer-sz))));
    SkipSpacesAndLineEnd();
}

// ------------------------------------------------------------------------------------------------
// Recursive MD5 parsing function
bool MD5Parser::ParseSection(Section& out)
{
    // store the current line number for use in error messages
    out.iLineNumber = lineNumber;

    // first parse the name of the section
    char* sz = buffer;
    while (!IsSpaceOrNewLine( *buffer))buffer++;
    out.mName = std::string(sz,(uintptr_t)(buffer-sz));
    SkipSpaces();

    bool running = true;
    while (running)    {
        if ('{' == *buffer)    {
            // it is a normal section so read all lines
            buffer++;
            bool run = true;
            while (run)
            {
                if (!SkipSpacesAndLineEnd()) {
                    return false; // seems this was the last section
                }
                if ('}' == *buffer)    {
                    buffer++;
                    break;
                }

                out.mElements.push_back(Element());
                Element& elem = out.mElements.back();

                elem.iLineNumber = lineNumber;
                elem.szStart = buffer;

                // terminate the line with zero
                while (!IsLineEnd( *buffer))buffer++;
                if (*buffer) {
                    ++lineNumber;
                    *buffer++ = '\0';
                }
            }
            break;
        }
        else if (!IsSpaceOrNewLine(*buffer))    {
            // it is an element at global scope. Parse its value and go on
            sz = buffer;
            while (!IsSpaceOrNewLine( *buffer++)) {}
            out.mGlobalValue = std::string(sz,(uintptr_t)(buffer-sz));
            continue;
        }
        break;
    }
    return SkipSpacesAndLineEnd();
}

// ------------------------------------------------------------------------------------------------
// Some dirty macros just because they're so funny and easy to debug

// skip all spaces ... handle EOL correctly
#define    AI_MD5_SKIP_SPACES()  if (!SkipSpaces(&sz)) \
    MD5Parser::ReportWarning("Unexpected end of line",(*eit).iLineNumber);

    // read a triple float in brackets: (1.0 1.0 1.0)
#define AI_MD5_READ_TRIPLE(vec) \
    AI_MD5_SKIP_SPACES(); \
    if ('(' != *sz++) \
        MD5Parser::ReportWarning("Unexpected token: ( was expected",(*eit).iLineNumber); \
    AI_MD5_SKIP_SPACES(); \
    sz = fast_atof_move(sz,(float&)vec.x); \
    AI_MD5_SKIP_SPACES(); \
    sz = fast_atof_move(sz,(float&)vec.y); \
    AI_MD5_SKIP_SPACES(); \
    sz = fast_atof_move(sz,(float&)vec.z); \
    AI_MD5_SKIP_SPACES(); \
    if (')' != *sz++) \
        MD5Parser::ReportWarning("Unexpected token: ) was expected",(*eit).iLineNumber);

    // parse a string, enclosed in quotation marks or not
#define AI_MD5_PARSE_STRING(out) \
    bool bQuota = (*sz == '\"'); \
    const char* szStart = sz; \
    while (!IsSpaceOrNewLine(*sz))++sz; \
    const char* szEnd = sz; \
    if (bQuota) { \
        szStart++; \
        if ('\"' != *(szEnd-=1)) { \
            MD5Parser::ReportWarning("Expected closing quotation marks in string", \
                (*eit).iLineNumber); \
            continue; \
        } \
    } \
    out.length = (size_t)(szEnd - szStart); \
    ::memcpy(out.data,szStart,out.length); \
    out.data[out.length] = '\0';

// ------------------------------------------------------------------------------------------------
// .MD5MESH parsing function
MD5MeshParser::MD5MeshParser(SectionList& mSections)
{
    DefaultLogger::get()->debug("MD5MeshParser begin");

    // now parse all sections
    for (SectionList::const_iterator iter =  mSections.begin(), iterEnd = mSections.end();iter != iterEnd;++iter){
        if ( (*iter).mName == "numMeshes")    {
            mMeshes.reserve(::strtol10((*iter).mGlobalValue.c_str()));
        }
        else if ( (*iter).mName == "numJoints")    {
            mJoints.reserve(::strtol10((*iter).mGlobalValue.c_str()));
        }
        else if ((*iter).mName == "joints")    {
            // "origin"    -1 ( -0.000000 0.016430 -0.006044 ) ( 0.707107 0.000000 0.707107 )
            for (ElementList::const_iterator eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end();eit != eitEnd; ++eit){
                mJoints.push_back(BoneDesc());
                BoneDesc& desc = mJoints.back();

                const char* sz = (*eit).szStart;
                AI_MD5_PARSE_STRING(desc.mName);
                AI_MD5_SKIP_SPACES();

                // negative values, at least -1, is allowed here
                desc.mParentIndex = (int)strtol10s(sz,&sz);

                AI_MD5_READ_TRIPLE(desc.mPositionXYZ);
                AI_MD5_READ_TRIPLE(desc.mRotationQuat); // normalized quaternion, so w is not there
            }
        }
        else if ((*iter).mName == "mesh")    {
            mMeshes.push_back(MeshDesc());
            MeshDesc& desc = mMeshes.back();

            for (ElementList::const_iterator eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end();eit != eitEnd; ++eit){
                const char* sz = (*eit).szStart;

                // shader attribute
                if (TokenMatch(sz,"shader",6))    {
                    AI_MD5_SKIP_SPACES();
                    AI_MD5_PARSE_STRING(desc.mShader);
                }
                // numverts attribute
                else if (TokenMatch(sz,"numverts",8))    {
                    AI_MD5_SKIP_SPACES();
                    desc.mVertices.resize(strtol10(sz));
                }
                // numtris attribute
                else if (TokenMatch(sz,"numtris",7))    {
                    AI_MD5_SKIP_SPACES();
                    desc.mFaces.resize(strtol10(sz));
                }
                // numweights attribute
                else if (TokenMatch(sz,"numweights",10))    {
                    AI_MD5_SKIP_SPACES();
                    desc.mWeights.resize(strtol10(sz));
                }
                // vert attribute
                // "vert 0 ( 0.394531 0.513672 ) 0 1"
                else if (TokenMatch(sz,"vert",4))    {
                    AI_MD5_SKIP_SPACES();
                    const unsigned int idx = ::strtol10(sz,&sz);
                    AI_MD5_SKIP_SPACES();
                    if (idx >= desc.mVertices.size())
                        desc.mVertices.resize(idx+1);

                    VertexDesc& vert = desc.mVertices[idx];
                    if ('(' != *sz++)
                        MD5Parser::ReportWarning("Unexpected token: ( was expected",(*eit).iLineNumber);
                    AI_MD5_SKIP_SPACES();
                    sz = fast_atof_move(sz,(float&)vert.mUV.x);
                    AI_MD5_SKIP_SPACES();
                    sz = fast_atof_move(sz,(float&)vert.mUV.y);
                    AI_MD5_SKIP_SPACES();
                    if (')' != *sz++)
                        MD5Parser::ReportWarning("Unexpected token: ) was expected",(*eit).iLineNumber);
                    AI_MD5_SKIP_SPACES();
                    vert.mFirstWeight = ::strtol10(sz,&sz);
                    AI_MD5_SKIP_SPACES();
                    vert.mNumWeights = ::strtol10(sz,&sz);
                }
                // tri attribute
                // "tri 0 15 13 12"
                else if (TokenMatch(sz,"tri",3)) {
                    AI_MD5_SKIP_SPACES();
                    const unsigned int idx = strtol10(sz,&sz);
                    if (idx >= desc.mFaces.size())
                        desc.mFaces.resize(idx+1);

                    aiFace& face = desc.mFaces[idx];
                    face.mIndices = new unsigned int[face.mNumIndices = 3];
                    for (unsigned int i = 0; i < 3;++i)    {
                        AI_MD5_SKIP_SPACES();
                        face.mIndices[i] = strtol10(sz,&sz);
                    }
                }
                // weight attribute
                // "weight 362 5 0.500000 ( -3.553583 11.893474 9.719339 )"
                else if (TokenMatch(sz,"weight",6))    {
                    AI_MD5_SKIP_SPACES();
                    const unsigned int idx = strtol10(sz,&sz);
                    AI_MD5_SKIP_SPACES();
                    if (idx >= desc.mWeights.size())
                        desc.mWeights.resize(idx+1);

                    WeightDesc& weight = desc.mWeights[idx];
                    weight.mBone = strtol10(sz,&sz);
                    AI_MD5_SKIP_SPACES();
                    sz = fast_atof_move(sz,weight.mWeight);
                    AI_MD5_READ_TRIPLE(weight.vOffsetPosition);
                }
            }
        }
    }
    DefaultLogger::get()->debug("MD5MeshParser end");
}

// ------------------------------------------------------------------------------------------------
// .MD5ANIM parsing function
MD5AnimParser::MD5AnimParser(SectionList& mSections)
{
    DefaultLogger::get()->debug("MD5AnimParser begin");

    fFrameRate = 24.0f;
    mNumAnimatedComponents = 0xffffffff;
    for (SectionList::const_iterator iter =  mSections.begin(), iterEnd = mSections.end();iter != iterEnd;++iter) {
        if ((*iter).mName == "hierarchy")    {
            // "sheath"    0 63 6
            for (ElementList::const_iterator eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end();eit != eitEnd; ++eit) {
                mAnimatedBones.push_back ( AnimBoneDesc () );
                AnimBoneDesc& desc = mAnimatedBones.back();

                const char* sz = (*eit).szStart;
                AI_MD5_PARSE_STRING(desc.mName);
                AI_MD5_SKIP_SPACES();

                // parent index - negative values are allowed (at least -1)
                desc.mParentIndex = ::strtol10s(sz,&sz);

                // flags (highest is 2^6-1)
                AI_MD5_SKIP_SPACES();
                if (63 < (desc.iFlags = ::strtol10(sz,&sz))){
                    MD5Parser::ReportWarning("Invalid flag combination in hierarchy section",(*eit).iLineNumber);
                }
                AI_MD5_SKIP_SPACES();

                // index of the first animation keyframe component for this joint
                desc.iFirstKeyIndex = ::strtol10(sz,&sz);
            }
        }
        else if ((*iter).mName == "baseframe")    {
            // ( -0.000000 0.016430 -0.006044 ) ( 0.707107 0.000242 0.707107 )
            for (ElementList::const_iterator eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end(); eit != eitEnd; ++eit) {
                const char* sz = (*eit).szStart;

                mBaseFrames.push_back ( BaseFrameDesc () );
                BaseFrameDesc& desc = mBaseFrames.back();

                AI_MD5_READ_TRIPLE(desc.vPositionXYZ);
                AI_MD5_READ_TRIPLE(desc.vRotationQuat);
            }
        }
        else if ((*iter).mName ==  "frame")    {
            if (!(*iter).mGlobalValue.length())    {
                MD5Parser::ReportWarning("A frame section must have a frame index",(*iter).iLineNumber);
                continue;
            }

            mFrames.push_back ( FrameDesc () );
            FrameDesc& desc = mFrames.back();
            desc.iIndex = strtol10((*iter).mGlobalValue.c_str());

            // we do already know how much storage we will presumably need
            if (0xffffffff != mNumAnimatedComponents)
                desc.mValues.reserve(mNumAnimatedComponents);

            // now read all elements (continous list of floats)
            for (ElementList::const_iterator eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end(); eit != eitEnd; ++eit){
                const char* sz = (*eit).szStart;
                while (SkipSpacesAndLineEnd(&sz))    {
                    float f;sz = fast_atof_move(sz,f);
                    desc.mValues.push_back(f);
                }
            }
        }
        else if ((*iter).mName == "numFrames")    {
            mFrames.reserve(strtol10((*iter).mGlobalValue.c_str()));
        }
        else if ((*iter).mName == "numJoints")    {
            const unsigned int num = strtol10((*iter).mGlobalValue.c_str());
            mAnimatedBones.reserve(num);

            // try to guess the number of animated components if that element is not given
            if (0xffffffff == mNumAnimatedComponents)
                mNumAnimatedComponents = num * 6;
        }
        else if ((*iter).mName == "numAnimatedComponents")    {
            mAnimatedBones.reserve( strtol10((*iter).mGlobalValue.c_str()));
        }
        else if ((*iter).mName == "frameRate")    {
            fast_atof_move((*iter).mGlobalValue.c_str(),fFrameRate);
        }
    }
    DefaultLogger::get()->debug("MD5AnimParser end");
}

// ------------------------------------------------------------------------------------------------
// .MD5CAMERA parsing function
MD5CameraParser::MD5CameraParser(SectionList& mSections)
{
    DefaultLogger::get()->debug("MD5CameraParser begin");
    fFrameRate = 24.0f;

    for (SectionList::const_iterator iter =  mSections.begin(), iterEnd = mSections.end();iter != iterEnd;++iter) {
        if ((*iter).mName == "numFrames")    {
            frames.reserve(strtol10((*iter).mGlobalValue.c_str()));
        }
        else if ((*iter).mName == "frameRate")    {
            fFrameRate = fast_atof ((*iter).mGlobalValue.c_str());
        }
        else if ((*iter).mName == "numCuts")    {
            cuts.reserve(strtol10((*iter).mGlobalValue.c_str()));
        }
        else if ((*iter).mName == "cuts")    {
            for (ElementList::const_iterator eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end(); eit != eitEnd; ++eit){
                cuts.push_back(strtol10((*eit).szStart)+1);
            }
        }
        else if ((*iter).mName == "camera")    {
            for (ElementList::const_iterator eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end(); eit != eitEnd; ++eit){
                const char* sz = (*eit).szStart;

                frames.push_back(CameraAnimFrameDesc());
                CameraAnimFrameDesc& cur = frames.back();
                AI_MD5_READ_TRIPLE(cur.vPositionXYZ);
                AI_MD5_READ_TRIPLE(cur.vRotationQuat);
                AI_MD5_SKIP_SPACES();
                cur.fFOV = fast_atof(sz);
            }
        }
    }
    DefaultLogger::get()->debug("MD5CameraParser end");
}

