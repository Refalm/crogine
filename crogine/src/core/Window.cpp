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

#include <crogine/core/Window.hpp>
#include <crogine/core/Log.hpp>
#include <crogine/core/App.hpp>
#include <crogine/core/Cursor.hpp>
#include <crogine/detail/SDLResource.hpp>

#include <SDL.h>
#include <SDL_video.h>

#include "../detail/GLCheck.hpp"
#include "DefaultLoadingScreen.hpp"

#include <algorithm>

using namespace cro;

namespace
{

}

Window::Window()
    : m_window              (nullptr),
    m_threadContext         (nullptr),
    m_mainContext           (nullptr),
    m_fullscreen            (false),
    m_multisamplingEnabled  (false),
    m_cursor                (nullptr)
{

}

Window::~Window()
{
    destroy();
}

//public
bool Window::create(std::uint32_t width, std::uint32_t height, const std::string& title, std::uint32_t styleFlags)
{
    if (!Detail::SDLResource::valid()) return false;
    
    destroy();

#ifdef PLATFORM_MOBILE
    fullscreen = true;
    borderless = true;
#endif //PLATFORM_MOBILE

    int styleMask = SDL_WINDOW_OPENGL | styleFlags;

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    m_window = SDL_CreateWindow(title.c_str(),SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, styleMask);

    if (!m_window)
    {
        Logger::log("failed creating Window");
        return false;
    }
    else
    {
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

        //for some reason this has to be done first on macOS
#ifdef __APPLE__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#endif

        m_threadContext = SDL_GL_CreateContext(m_window);
        m_mainContext = SDL_GL_CreateContext(m_window);
        //SDL_GL_MakeCurrent(m_window, m_mainContext);
        
#ifndef __APPLE__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#endif

        int maj, min;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &maj);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &min);

        if (maj != 4 || min != 1)
        {
            Logger::log("Unable to create requested context version");
            Logger::log("Returned version was: " + std::to_string(maj) + "." + std::to_string(min));
        }
        LOG("Created OpenGL context version: " + std::to_string(maj) + "." + std::to_string(min), Logger::Type::Info);
    }
    return true;
}

void Window::setVsyncEnabled(bool enabled)
{
    if (m_mainContext)
    {
        if (SDL_GL_SetSwapInterval(enabled ? 1 : 0) != 0)
        {
            LogE << SDL_GetError() << std::endl;
        }
    }
}

bool Window::getVsyncEnabled() const
{
    return SDL_GL_GetSwapInterval() != 0;
}

void Window::setMultisamplingEnabled(bool enabled)
{
    if (enabled != m_multisamplingEnabled)
    {
        if (enabled)
        {
            glCheck(glEnable(GL_MULTISAMPLE));
        }
        else
        {
            glCheck(glDisable(GL_MULTISAMPLE));
        }
        m_multisamplingEnabled = enabled;
    }
}

bool Window::getMultisamplingEnabled() const
{
    return m_multisamplingEnabled;
}

void Window::clear()
{
    //glClearColor(1.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::display()
{
    SDL_GL_SwapWindow(m_window);
}

bool Window::pollEvent(Event& evt)
{
    return SDL_PollEvent(&evt);
}

void Window::close()
{
    destroy();
}

glm::uvec2 Window::getSize() const
{
    CRO_ASSERT(m_window, "window not created");
    std::int32_t x, y;
#ifdef PLATFORM_MOBILE
    SDL_GL_GetDrawableSize(m_window, &x, &y);
#else
    SDL_GetWindowSize(m_window, &x, &y);
#endif //PLATFORM_MOBILE
    return { x, y };
}

void Window::setSize(glm::uvec2 size)
{
    CRO_ASSERT(m_window, "window not created");
    SDL_SetWindowSize(m_window, size.x, size.y);
    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void Window::setFullScreen(bool fullscreen)
{
    CRO_ASSERT(m_window, "window not created");
    if (SDL_SetWindowFullscreen(m_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0)
    {
        m_fullscreen = fullscreen;
        if (!fullscreen)
        {
            SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
    }
}

void Window::setPosition(std::int32_t x, std::int32_t y)
{
    CRO_ASSERT(m_window, "window not created");
    if (x < 0) x = SDL_WINDOWPOS_CENTERED;
    if (y < 0) y = SDL_WINDOWPOS_CENTERED;
    SDL_SetWindowPosition(m_window, x, y);
}

void Window::setIcon(const std::uint8_t* data)
{
    CRO_ASSERT(m_window, "window not created");
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void*)(data), 16, 16, 32, 16 * 4, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if (surface)
    {
        SDL_SetWindowIcon(m_window, surface);
        SDL_FreeSurface(surface);
    }
    else
    {
        Logger::log("Failed creating icon from pixel data", Logger::Type::Error);
    }
}

const std::vector<glm::uvec2>& Window::getAvailableResolutions() const
{
    CRO_ASSERT(m_window, "window not created");
    if (m_resolutions.empty())
    {
        auto modeCount = SDL_GetNumDisplayModes(0);
        if (modeCount > 0)
        {
            SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };
            for (auto i = 0; i < modeCount; ++i)
            {
                if (SDL_GetDisplayMode(0, i, &mode) == 0)
                {
                    if (SDL_BITSPERPIXEL(mode.format) == 24)
                    {
                        m_resolutions.emplace_back(mode.w, mode.h);
                    }
                }
            }
            m_resolutions.erase(std::unique(std::begin(m_resolutions), std::end(m_resolutions)), std::end(m_resolutions));
        }
        else
        {
            std::string err(SDL_GetError());
            Logger::log("failed retrieving available resolutions: " + err, Logger::Type::Error, Logger::Output::All);
        }
    }
    return m_resolutions;
}

void Window::setTitle(const std::string& title)
{
    CRO_ASSERT(m_window, "window not created");
    SDL_SetWindowTitle(m_window, title.c_str());
}

namespace
{
    struct ThreadData final
    {
        SDL_Window* window = nullptr;
        SDL_GLContext context = nullptr;
        SDL_atomic_t threadFlag;
        LoadingScreen* loadingScreen = nullptr;
    };

    int loadingDisplayFunc(void* data)
    {
        ThreadData* threadData = static_cast<ThreadData*>(data);
        SDL_GL_MakeCurrent(threadData->window, threadData->context);

        while (SDL_AtomicGet(&threadData->threadFlag) != 1)
        {
            threadData->loadingScreen->update();
            glCheck(glClear(GL_COLOR_BUFFER_BIT));
            threadData->loadingScreen->draw();
            SDL_GL_SwapWindow(threadData->window);
        }

        SDL_GL_MakeCurrent(threadData->window, nullptr);
        return 0;
    }
}

void Window::loadResources(const std::function<void()>& loader)
{
    if (!m_loadingScreen)
    {
        m_loadingScreen = std::make_unique<DefaultLoadingScreen>();
    }
  
#ifdef PLATFORM_DESKTOP
    //create thread
    ThreadData data;
    data.context = m_threadContext;
    data.window = m_window;
    data.threadFlag.value = 0;
    data.loadingScreen = m_loadingScreen.get();

    SDL_Thread* thread = SDL_CreateThread(loadingDisplayFunc, "Loading Thread", static_cast<void*>(&data));

    loader();
    glFinish(); //make sure to wait for gl stuff to finish before continuing

    SDL_AtomicIncRef(&data.threadFlag);

    std::int32_t result = 0;
    SDL_WaitThread(thread, &result);

    //SDL_GL_MakeCurrent(m_window, m_mainContext);

    App::getInstance().resetFrameTime();
#else

    //android doesn't appear to like running the thread - so we'll display the loading screen once
    //before loading resources
    m_loadingScreen->update();
    glCheck(glClear(GL_COLOR_BUFFER_BIT));
    m_loadingScreen->draw();
    SDL_GL_SwapWindow(m_window);

    loader();

#endif //PLATFORM_DESKTOP
}

void Window::setMouseCaptured(bool captured)
{
    SDL_SetRelativeMouseMode(captured ? SDL_TRUE : SDL_FALSE);
    //SDL_CaptureMouse(captured ? SDL_TRUE : SDL_FALSE);
}

bool Window::getMouseCaptured() const
{
    return (SDL_GetRelativeMouseMode() == SDL_TRUE);
}

void Window::setCursor(const Cursor* cursor)
{
    if (cursor == m_cursor)
    {
        return;
    }

    if (cursor && cursor->m_cursor == nullptr)
    {
        return;
    }

    if (m_cursor)
    {
        m_cursor->m_inUse = false;
    }

    m_cursor = cursor;

    if (m_cursor)
    {
        m_cursor->m_inUse = true;
        SDL_SetCursor(m_cursor->m_cursor);
    }
    else
    {
        SDL_SetCursor(nullptr);
    }
}

const Cursor* Window::getCursor() const
{
    return m_cursor;
}

//private
void Window::destroy()
{
    if (m_mainContext)
    {
        m_loadingScreen.reset(); //delete this while we still have a valid context!
        SDL_GL_DeleteContext(m_mainContext);
        m_mainContext = nullptr;
    }

    if(m_threadContext)
    {
        SDL_GL_DeleteContext(m_threadContext);
        m_threadContext = nullptr;
    }

    if (m_window)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
}