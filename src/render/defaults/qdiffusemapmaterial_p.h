/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT3D_RENDER_QDIFFUSEMAPMATERIAL_P_H
#define QT3D_RENDER_QDIFFUSEMAPMATERIAL_P_H

#include <Qt3DRenderer/private/qmaterial_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3D {

class QEffect;
class QTexture;
class QTechnique;
class QParameter;
class QShaderProgram;
class QRenderPass;
class QParameterMapping;

class QDiffuseMapMaterial;

class QDiffuseMapMaterialPrivate : public QMaterialPrivate
{
    QDiffuseMapMaterialPrivate(QDiffuseMapMaterial *qq);

    void init();

    QEffect *m_diffuseMapEffect;
    QTexture *m_diffuseTexture;
    QParameter *m_ambientParameter;
    QParameter *m_diffuseParameter;
    QParameter *m_specularParameter;
    QParameter *m_shininessParameter;
    QParameter *m_textureScaleParameter;
    QTechnique *m_diffuseMapGL3Technique;
    QTechnique *m_diffuseMapGL2Technique;
    QTechnique *m_diffuseMapES2Technique;
    QRenderPass *m_diffuseMapGL3RenderPass;
    QRenderPass *m_diffuseMapGL2RenderPass;
    QRenderPass *m_diffuseMapES2RenderPass;
    QShaderProgram *m_diffuseMapGL3Shader;
    QShaderProgram *m_diffuseMapGL2ES2Shader;

    Q_DECLARE_PUBLIC(QDiffuseMapMaterial)
};

} // Qt3D

QT_END_NAMESPACE

#endif // QT3D_RENDER_QDIFFUSEAPMATERIAL_P_H
