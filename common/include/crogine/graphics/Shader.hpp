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

#ifndef CRO_SHADER_HPP_
#define CRO_SHADER_HPP_

#include <crogine/Config.hpp>
#include <crogine/detail/SDLResource.hpp>
#include <crogine/detail/Types.hpp>

#include <string>

namespace cro
{
    /*!
    \bref Encapsulates a compiled/linked shader program
    */
    class CRO_EXPORT_API Shader : public Detail::SDLResource
    {
    public:
        Shader();
        ~Shader();

        Shader(const Shader&) = delete;
        Shader(const Shader&&) = delete;
        Shader& operator = (const Shader&) = delete;
        Shader& operator = (const Shader&&) = delete;

        /*!
        \brief Attempts to load the shader source from given files on disk.
        \param vertexPath Path to the file containing vertex shader
        \param fragmentPath Path to file containing source for fragment shader
        \returns true on success, else returns false
        */
        bool loadFromFile(const std::string& vertexPath, const std::string& fragmentPath);

        /*!
        \brief Attempts to load the shader source from given strings
        \param vertex A string containing the source of the vertex shader
        \param fragment A string containing the source for the fragment shader
        \returns true on success, else returns false
        */
        bool loadFromString(const std::string& vertex, const std::string& fragment);

        /*!
        \brief Returns the opengl handle for the shader program
        */
        uint32 getGLHandle() const;

    private:
        uint32 m_handle;

        std::string parseFile(const std::string&);
    };
}

#endif //CRO_SHADER_HPP_