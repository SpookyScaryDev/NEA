#pragma once
#include <Application/Application.h>
#include <string>

// Show an error box with tracing information.
#define ERROR(message) Application::GetApp()->Error( std::string("Error at line ") + std::to_string(__LINE__) + " of " + __FILE__ + " during " + __FUNCTION__ ":\n" + message); __debugbreak();

// Display an error message if the expression is false.
#define ASSERT(expression, message )                   \
    if (expression) {}                                 \
    else {                                             \
        ERROR("Assertion Failed: " + message); \
    }