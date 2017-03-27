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

template <class T>
bool Rectangle<T>::intersects(Rectangle<T> other, Rectangle<T>& overlap) const
{
    T r1MinX = std::min(left, static_cast<T>(left + width));
    T r1MaxX = std::max(left, static_cast<T>(left + width));
    T r1MinY = std::min(bottom, static_cast<T>(bottom + height));
    T r1MaxY = std::max(bottom, static_cast<T>(bottom + height));

    T r2MinX = std::min(other.left, static_cast<T>(other.left + other.width));
    T r2MaxX = std::max(other.left, static_cast<T>(other.left + other.width));
    T r2MinY = std::min(other.bottom, static_cast<T>(other.bottom + other.height));
    T r2MaxY = std::max(other.bottom, static_cast<T>(other.bottom + other.height));

    T interLeft = std::max(r1MinX, r2MinX);
    T interTop = std::max(r1MinY, r2MinY);
    T interRight = std::min(r1MaxX, r2MaxX);
    T interBottom = std::min(r1MaxY, r2MaxY);

    if ((interLeft < interRight) && (interTop < interBottom))
    {
        overlap = Rectangle<T>(interLeft, interTop, interRight - interLeft, interBottom - interTop);
        return true;
    }
    else
    {
        overlap = Rectangle<T>(0, 0, 0, 0);
        return false;
    }

    return false;
}

template <class T>
bool Rectangle<T>::contains(Rectangle<T> second) const
{
    if (second.left < left) return false;
    if (second.bottom < bottom) return false;
    if (second.left + second.width > left + width) return false;
    if (second.bottom + second.height > bottom + height) return false;

    return true;
}

template <class T>
bool Rectangle<T>::contains(glm::vec2 point) const
{
    T x = static_cast<T>(point.x);
    T y = static_cast<T>(point.y);

    T minX = std::min(left, static_cast<T>(left + width));
    T maxX = std::max(left, static_cast<T>(left + width));
    T minY = std::min(bottom, static_cast<T>(bottom + height));
    T maxY = std::max(bottom, static_cast<T>(bottom + height));

    return (x >= minX) && (x < maxX) && (y >= minY) && (y < maxY);

    return false;
}