/*-----------------------------------------------------------------------

Matt Marchant 2022
http://trederia.blogspot.com

Super Video Golf - zlib licence.

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

#include <crogine/graphics/Image.hpp>

class Social final
{
public:
    static bool isAvailable() { return false; }
    static cro::Image getUserIcon(std::uint64_t) { return cro::Image(); }
    static void findFriends() {}
    static void inviteFriends(std::uint64_t) {}
    static constexpr std::uint32_t IconSize = 64;
    static inline const std::string RSSFeed = "https://fallahn.itch.io/vga-golf/devlog.rss";
    static inline const std::string WebURL = "https://fallahn.itch.io/vga-golf";
};