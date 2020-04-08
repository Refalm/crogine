/*-----------------------------------------------------------------------

Matt Marchant 2017 - 2020
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

#pragma once

#include <crogine/Config.hpp>
#include <crogine/graphics/Rectangle.hpp>
#include <crogine/graphics/Colour.hpp>
#include <crogine/graphics/MaterialData.hpp>

#include <crogine/detail/glm/vec2.hpp>
#include <crogine/detail/glm/vec3.hpp>
#include <crogine/detail/glm/vec4.hpp>

#include <array>

namespace cro
{
    class Texture;
    /*!
    \brief 2D Sprite component.
    Sprites are rendered with a RenderSystem2D rather than
    a model renderer. The 2D renderer can be used on its own, or rendered
    over the top of a 3D scene, for example as a user interface. Sprites
    require their Entity to have a Transform component and a Drawable2D component.
    Sprites also require a SpriteSystem in the Scene to update their properties.
    */
    class CRO_EXPORT_API Sprite final
    {
    public:
        Sprite();

        /*!
        \brief Construct a sprite with the given Texture
        */
        explicit Sprite(const Texture&);

        /*!
        \brief Sets the texture used to render this sprite.
        By default the Sprite is resized to match the given Texture.
        Set resize to false to maintain the current size.
        */
        void setTexture(const Texture&, bool resize = true);

        /*!
        \brief Sets a sub-rectangle of the texture to draw.
        This effectively sets the UV coordinates of the sprite.
        \see setTexture()
        */
        void setTextureRect(FloatRect);

        /*!
        \brief Sets the colour of the sprite.
        This colour is multiplied with that of the texture
        when it is drawn.
        */
        void setColour(Colour);

        /*!
        \brief Returns a pointer to the Sprite's texture
        */
        const Texture* getTexture() const { return m_texture; }

        /*!
        \brief Returns the current Texture Rectangle
        */
        const FloatRect& getTextureRect() const { return m_textureRect; }

        /*!
        \brief Returns the current colour of the sprite
        */
        Colour getColour() const;

        /*!
        \brief Gets the current size of the sprite
        */
        glm::vec2 getSize() const { return { m_textureRect.width, m_textureRect.height }; }
        /*!

        \brief Returns the created by the active texture rectangle's size
        */
        FloatRect getTextureBounds() const { return { glm::vec2(0.f), getSize() }; }

        /*!
        \brief Maximum number of frames in an animation
        */
        static constexpr std::size_t MaxFrames = 64;

        /*!
        \brief Maximum number of animations per sprite
        */
        static constexpr std::size_t MaxAnimations = 32;

        /*!
        \brief Represents a single animation
        */
        struct Animation final
        {
            /*!
            \brief Maximum  length of animation id
            */
            static constexpr std::size_t MaxAnimationIdLength = 32;

            std::array<char, MaxAnimationIdLength> id = { {0} };
            std::array<FloatRect, MaxFrames> frames = {};
            std::size_t frameCount = 0;
            std::uint32_t loopStart = 0; //!< looped animations can jump to somewhere other than the beginning
            bool looped = false;
            float framerate = 12.f;
        };

        /*!
        \brief Returns the number of animations for this sprite when loaded
        from a sprite sheet definition file.
        */
        std::size_t getAnimationCount() const { return m_animationCount; }

        /*!
        /brief Returns a reference to the sprites animation array.
        Use getAnimationCount() to check how many of the animations are valid
        */
        std::array<Animation, MaxAnimations>& getAnimations() { return m_animations; }

        /*!
        /brief Returns a const reference to the sprites animation array.
        Use getAnimationCount() to check how many of the animations are valid
        */
        const std::array<Animation, MaxAnimations>& getAnimations() const { return m_animations; }

    private:
        FloatRect m_textureRect;
        const Texture* m_texture;
        Colour m_colour;
        bool m_dirty;

        std::size_t m_animationCount;
        std::array<Animation, MaxAnimations> m_animations;

        //if this was loaded from a sprite sheet the blend
        //mode was set here and needs to be forwarded to
        //the drawable component.
        bool m_overrideBlendMode;
        Material::BlendMode m_blendMode;

        friend class SpriteSystem;
        friend class SpriteAnimator;
        friend class SpriteSheet;
    };
}