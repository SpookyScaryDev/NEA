#pragma once

#include <Window/Window.h>
#include <Renderer/Renderer.h>
#include <string>

namespace Prototype {

// Singleton which the app inherits from.
class Application {
public:
                          Application(const char* name, int width, int height); // Calls Init.
    virtual               ~Application();                                       // Calls Shutdown.
    static Application*   GetApp();                                             // Retrieve the static game instance.
    void                  Run();
    virtual void          Update(float deltaTime) {};       // Called every frame.

    Window*               GetWindow() const;
    Renderer*             GetRenderer() const;

    void                  Error(const std::string message); // Show error message box. 

protected:
    bool                  mIsRunning;

private:
    void                  Init(const char* name, int width, int height);
    void                  Shutdown() const;

    static Application*   mApplication;

    Window*               mWindow;
    Renderer*             mRenderer;
};

}
