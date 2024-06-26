#include "entity/ticker.hpp"

#include "render/window.hpp"

namespace entity
{
    ticker::~ticker()
    {

    }

    void ticker::init(render::window* win)
    {
        width = win->swapchainExtent.width;
        height = win->swapchainExtent.height;

        glfwSetWindowUserPointer(win->win.get(), this);
        glfwSetScrollCallback(win->win.get(), [](GLFWwindow* w, double x, double y){
            ticker* t = (ticker*)glfwGetWindowUserPointer(w);
            t->on_scroll(x, y);
        });
        glfwSetCursorPosCallback(win->win.get(), [](GLFWwindow* w, double x, double y){
            ticker* t = (ticker*)glfwGetWindowUserPointer(w);
            t->on_cursor_pos(x, y);
        });
        glfwSetKeyCallback(win->win.get(), [](GLFWwindow* w, int key, int scancode, int action, int mods){
            ticker* t = (ticker*)glfwGetWindowUserPointer(w);
            t->on_key(key, scancode, action, mods);
        });
    }

    void ticker::tick(double dt)
    {

    }
}
