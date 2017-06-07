/*-----------------------------------------------------------------------

Matt Marchant 2017
http://trederia.blogspot.com

crogine - Zlib license.

This software is provided 'as-is', without any express or
implied warranty.In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions :

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.

-----------------------------------------------------------------------*/

#ifndef CRO_SHADER_UNLIT_HPP_
#define CRO_SHADER_UNLIT_HPP_

#include <string>

namespace cro
{
    namespace Shaders
    {
        namespace Unlit
        {
            const static std::string Vertex = R"(
                attribute vec4 a_position;
                #if defined(VERTEX_COLOUR)
                attribute vec4 a_colour;
                #endif
                #if defined(TEXTURED)
                attribute MED vec2 a_texCoord0;
                #endif
                #if defined(LIGHTMAPPED)
                attribute MED vec2 a_texCoord1;
                #endif

                #if defined(SKINNED)
                attribute vec4 a_boneIndices;
                attribute vec4 a_boneWeights;
                uniform mat4 u_boneMatrices[MAX_BONES];
                #endif

                #if defined(PROJECTIONS)
                #define MAX_PROJECTIONS 8
                uniform mat4 u_projectionMapMatrix[MAX_PROJECTIONS]; //VP matrices for texture projection
                uniform LOW int u_projectionMapCount; //how many to actually draw
                #endif

                uniform mat4 u_worldMatrix;
                uniform mat4 u_worldViewMatrix;               
                uniform mat4 u_projectionMatrix;
                #if defined (SUBRECTS)
                uniform MED vec4 u_subrect;
                #endif
                
                #if defined (VERTEX_COLOUR)
                varying LOW vec4 v_colour;
                #endif
                #if defined (TEXTURED)
                varying MED vec2 v_texCoord0;
                #endif
                #if defined (LIGHTMAPPED)
                varying MED vec2 v_texCoord1;
                #endif

                #if defined(PROJECTIONS)
                varying LOW vec4 v_projectionCoords[MAX_PROJECTIONS];
                #endif

                void main()
                {
                    mat4 wvp = u_projectionMatrix * u_worldViewMatrix;
                    vec4 position = a_position;

                #if defined(PROJECTIONS)
                    for(int i = 0; i < u_projectionMapCount; ++i)
                    {
                        v_projectionCoords[i] = u_projectionMapMatrix[i] * u_worldMatrix * a_position;
                    }
                #endif

                #if defined(SKINNED)
                	mat4 skinMatrix = u_boneMatrices[int(a_boneIndices.x)] * a_boneWeights.x;
                	skinMatrix += u_boneMatrices[int(a_boneIndices.y)] * a_boneWeights.y;
                	skinMatrix += u_boneMatrices[int(a_boneIndices.z)] * a_boneWeights.z;
                	skinMatrix += u_boneMatrices[int(a_boneIndices.w)] * a_boneWeights.w;
                	position = skinMatrix * position;
                #endif

                    gl_Position = wvp * position;

                #if defined (VERTEX_COLOUR)
                    v_colour = a_colour;
                #endif
                #if defined (TEXTURED)
                #if defined (SUBRECTS)
                    v_texCoord0 = u_subrect.xy + (a_texCoord0 * u_subrect.zw);
                #else
                    v_texCoord0 = a_texCoord0;                    
                #endif
                #endif
                #if defined (LIGHTMAPPED)
                    v_texCoord1 = a_texCoord1;
                #endif
                })";

            const static std::string Fragment = R"(
                #if defined (TEXTURED)
                uniform sampler2D u_diffuseMap;
                #endif
                #if defined (LIGHTMAPPED)
                uniform sampler2D u_lightMap;
                #endif
                #if defined(COLOURED)
                uniform LOW vec4 u_colour;
                #endif
                #if defined(PROJECTIONS)
                #define MAX_PROJECTIONS 8
                uniform sampler2D u_projectionMap;
                uniform LOW int u_projectionMapCount;
                #endif

                #if defined(RIMMING)
                uniform LOW vec4 u_rimColour;
                uniform LOW float u_rimFalloff;
                #endif

                #if defined (VERTEX_COLOUR)
                varying LOW vec4 v_colour;
                #endif
                #if defined (TEXTURED)
                varying MED vec2 v_texCoord0;
                #endif
                #if defined (LIGHTMAPPED)
                varying MED vec2 v_texCoord1;
                #endif                


                #if defined(PROJECTIONS)
                varying LOW vec4 v_projectionCoords[MAX_PROJECTIONS];
                #endif

                void main()
                {
                #if defined (VERTEX_COLOUR)
                    gl_FragColor = v_colour;
                #else
                    gl_FragColor = vec4(1.0);
                #endif
                #if defined (TEXTURED)
                    gl_FragColor *= texture2D(u_diffuseMap, v_texCoord0);
                #endif
                #if defined (LIGHTMAPPED)
                    gl_FragColor *= texture2D(u_lightMap, v_texCoord1);
                #endif

                #if defined(COLOURED)
                    gl_FragColor *= u_colour;
                #endif
                #if defined(PROJECTIONS)
                    for(int i = 0; i < u_projectionMapCount; ++i)
                    {
                        if(v_projectionCoords[i].w > 0.0)
                        {
                            vec2 coords = v_projectionCoords[i].xy / v_projectionCoords[i].w / 2.0 + 0.5;
                            gl_FragColor *= texture2D(u_projectionMap, coords);
                        }
                    }
                #endif
                #if defined (RIMMING)
                    LOW float rim = 1.0 - dot(normal, eyeDirection);
                    rim = smoothstep(u_rimFalloff, 1.0, rim);
                    //gl_FragColor.rgb = mix(gl_FragColor.rgb, u_rimColour.rgb, rim);
                    gl_FragColor += u_rimColour * rim ;//* 0.5;
                #endif
                })";
        }
    }
}

#endif //CRO_SHADER_UNLIT_HPP_