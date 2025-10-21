#pragma once

#include <utility>
#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <functional>

struct Point {
    float x;
    float y;

    Point operator-(const Point& rhs) const {
        return {x - rhs.x, y - rhs.y};
    }
    Point operator+(const Point& rhs) const {
        return {x + rhs.x, y + rhs.y};
    }
    Point operator*(const float rhs) const {
        return {x * rhs, y * rhs};
    }
    Point operator/(const float rhs) const {
        return {x / rhs, y / rhs};
    }
    Point& operator+=(const Point& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
};


enum class CommandType { MOVE, MOVE_REL, LINE, LINE_REL, QUAD, QUAD_REL, CUBIC, CUBIC_REL, CLOSE };
struct Command {
    CommandType type;
    Point to;
    Point control0;
    Point control1;

    explicit Command(const CommandType type, const Point& to = {}, const Point& control0 = {}, const Point& control1 = {})
        : type(type), to(to), control0(control0), control1(control1) {}
};

class Path {
public:

    void add(FT_Outline& outline, const Point& offset);
    std::string string() const;
    const std::vector<Command>& get_commands() const { return path; }

    std::pair<float, float> lowest() const;

    Path as_rel() const;
    Path& to_cubic();
    Path& transform(const std::function<std::pair<float, float>(float, float)>& tr);
    Path& reorder();
private:
    std::vector<Command> path;
    Point current_offset{};

    static int move_to(const FT_Vector* to, void* user);
    static int line_to(const FT_Vector* to, void* user);
    static int quad_to(const FT_Vector* control, const FT_Vector* to, void* user);
    static int cubic_to(const FT_Vector* control_one, const FT_Vector* control_two, const FT_Vector* to, void* user);
};
