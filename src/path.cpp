#include "path.h"

#include <limits>
#include <algorithm>
#include <fmt/format.h>
#include FT_OUTLINE_H

void Path::add(FT_Outline& outline, const Point& offset) {
    current_offset = offset;
    constexpr FT_Outline_Funcs callbacks = {move_to, line_to, quad_to, cubic_to, 0, 0};
    if (FT_Outline_Decompose(&outline, &callbacks, this))
        throw std::runtime_error("Outline get fail");
}

std::string Path::string() const {
    std::string pathstr;
    if (path.empty())
        return pathstr;
    for (const auto& [type, to, control0, control1] : path) {
        switch (type) {
            case CommandType::MOVE: pathstr += fmt::format("M {} {} ", to.x, to.y); break;
            case CommandType::LINE: pathstr += fmt::format("L {} {} ", to.x, to.y); break;
            case CommandType::QUAD: pathstr += fmt::format("Q {} {} {} {} ", control0.x, control0.y, to.x, to.y); break;
            case CommandType::CUBIC: pathstr += fmt::format("C {} {} {} {} {} {} ", control0.x, control0.y, control1.x, control1.y, to.x, to.y); break;
            case CommandType::MOVE_REL: pathstr += fmt::format("m {} {} ", to.x, to.y); break;
            case CommandType::LINE_REL: pathstr += fmt::format("l {} {} ", to.x, to.y); break;
            case CommandType::QUAD_REL: pathstr += fmt::format("q {} {} {} {} ", control0.x, control0.y, to.x, to.y); break;
            case CommandType::CUBIC_REL: pathstr += fmt::format("c {} {} {} {} {} {} ", control0.x, control0.y, control1.x, control1.y, to.x, to.y); break;
            case CommandType::CLOSE: pathstr += "Z "; break;
        }
    }
    pathstr.pop_back();
    return pathstr;
}

Path Path::as_rel() const {
    Path rel_path;

    Point current_pos{};
    Point start_of_path{};

    for (const auto& cmd : path) {
        switch (cmd.type) {
            case CommandType::MOVE: {
                Command rel_cmd(CommandType::MOVE_REL, cmd.to - current_pos);
                rel_path.path.emplace_back(rel_cmd);
                current_pos += rel_cmd.to;
                start_of_path = current_pos;
                break;
            }
            case CommandType::LINE: {
                Command rel_cmd(CommandType::LINE_REL, cmd.to - current_pos);
                rel_path.path.emplace_back(rel_cmd);
                current_pos += rel_cmd.to;
                break;
            }
            case CommandType::QUAD: {
                Command rel_cmd(CommandType::QUAD_REL, cmd.to - current_pos, cmd.control0 - current_pos);
                rel_path.path.emplace_back(rel_cmd);
                current_pos += rel_cmd.to;
                break;
            }
            case CommandType::CUBIC: {
                Command rel_cmd(CommandType::CUBIC_REL, cmd.to - current_pos, cmd.control0 - current_pos, cmd.control1 - current_pos);
                rel_path.path.emplace_back(rel_cmd);
                current_pos += rel_cmd.to;
                break;
            }
            case CommandType::CLOSE:
                rel_path.path.push_back(cmd);
                current_pos = start_of_path;
                break;
            default:
                throw std::invalid_argument("rel called on relative path");
        }

    }

    return rel_path;
}

Path& Path::to_cubic() {
    Point current_pos{};
    for (auto& cmd : path) {
        if (cmd.type == CommandType::QUAD || cmd.type == CommandType::QUAD_REL)
            cmd = Command(
                cmd.type == CommandType::QUAD ? CommandType::CUBIC : CommandType::CUBIC_REL,
                cmd.to,
                (current_pos + (cmd.control0 - current_pos) * (2.0 / 3.0)),
                (cmd.to + (cmd.control0 - cmd.to) * (2.0 / 3.0))
            );

        if (cmd.type != CommandType::CLOSE)
            current_pos = cmd.to;
    }

    return *this;
}


Path& Path::transform(const std::function<std::pair<float, float>(float, float)>& tr) {
    auto tp = [&tr](Point& p) {
        auto [x, y] = tr(p.x, p.y);
        p.x = x;
        p.y = y;
    };
    for (auto& cmd : path) {
        switch (cmd.type) {
            case CommandType::CLOSE:
                break;
            case CommandType::CUBIC:
            case CommandType::CUBIC_REL:
                tp(cmd.control1);
            case CommandType::QUAD:
            case CommandType::QUAD_REL:
                tp(cmd.control0);
            default:
                tp(cmd.to);
                break;
        }
    }
    return *this;
}



inline std::vector<Command> make_clockwise(const std::vector<Command>& sub) {
    std::vector<Command> new_path{sub[0]};
    std::vector<Command> reversed(sub.begin() + 1, sub.end());
    std::reverse(reversed.begin(), reversed.end());
    for (int i = 0; i < reversed.size(); i++) {
        const Command cmd = reversed[i];
        Point where_we_were{};
        if (i + 1 == reversed.size()) {
            where_we_were = sub[0].to;
        } else {
            where_we_were = reversed[i + 1].to;
        }

        Command new_cmd(cmd.type, where_we_were);

        switch (cmd.type) {
            case CommandType::CUBIC:
            case CommandType::CUBIC_REL:
                new_cmd.control0 = cmd.control1;
                new_cmd.control1 = cmd.control0;
                break;
            case CommandType::QUAD:
            case CommandType::QUAD_REL:
                throw std::invalid_argument("clockwise called on path with Q");
            case CommandType::CLOSE:
                throw std::invalid_argument("clockwise called on path with Z");
            default:
                break;
        }
        new_path.push_back(new_cmd);
    }
    return new_path;
}


inline bool is_clockwise(const std::vector<Command>& cmds) {
    float det = 0.0;
    for (int i = 0; i < cmds.size() - 1; i++) {
        if (cmds[i].type == CommandType::CLOSE)
            throw std::invalid_argument("clockwise called on path with Z");
        det += (cmds[i].to.x * cmds[i + 1].to.y) - (cmds[i].to.y * cmds[i + 1].to.x);
    }
    return det > 0.0;
}


inline bool is_top_left_of(const Point& a, const Point& b) {
    const float a_norm = a.x * a.x + a.y * a.y;
    const float b_norm = b.x * b.x + b.y * b.y;
    return a.y < b.y || (a_norm == b_norm && a.x < b.x);
}



Path& Path::reorder() {
    std::vector<std::vector<Command>> separated;
    std::vector<Command> current_path;
    for (const auto& command : path) {
        if (!current_path.empty() && command.type == CommandType::MOVE) {
            separated.push_back(current_path);
            current_path.clear();
        }
        current_path.push_back(command);
    }
    if (!current_path.empty())
        separated.push_back(current_path);

    std::vector<std::pair<std::vector<Command>, Point>> reordered;
    for (auto& sep : separated) {
        Point left = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        int leftest_idx = -1;

        for (int i = 0; i < sep.size(); i++) {
            const Command& cmd = sep[i];
            if (cmd.type != CommandType::CLOSE) {
                if (is_top_left_of(cmd.to, left)) {
                    left = cmd.to;
                    leftest_idx = i;
                }
            }
        }

        std::vector<Command> as_start_left{Command(CommandType::MOVE, left)};
        as_start_left.insert(as_start_left.end(), sep.begin() + leftest_idx + 1, sep.end());
        as_start_left.insert(as_start_left.end(), sep.begin() + 1, sep.begin() + leftest_idx + 1);

        reordered.emplace_back(as_start_left, left);
    }

    std::sort(reordered.begin(), reordered.end(), [](const auto& a, const auto& b) {
        if (a.second.y == b.second.y)
            return a.second.x < b.second.x;
        return a.second.y < b.second.y;
    });

    path.clear();

    bool first_done = false;
    bool flip_cardinality = false;
    for (auto& [cmds, _] : reordered) {
        if (!first_done) {
            flip_cardinality = !is_clockwise(cmds);
            first_done = true;
        }

        if (flip_cardinality)
            cmds = make_clockwise(cmds);

        path.insert(path.end(), cmds.begin(), cmds.end());
    }


    return *this;
}


std::pair<float, float> Path::lowest() const {
    float x_min = std::numeric_limits<float>::max();
    float y_min = std::numeric_limits<float>::max();

    for (const auto& cmd : path) {
        switch (cmd.type) {
            case CommandType::CLOSE:
                break;
            case CommandType::CUBIC:
            case CommandType::CUBIC_REL:
                if (cmd.control1.x < x_min)
                    x_min = cmd.control1.x;
                if (cmd.control1.y < y_min)
                    y_min = cmd.control1.y;
            case CommandType::QUAD:
            case CommandType::QUAD_REL:
                if (cmd.control0.x < x_min)
                    x_min = cmd.control0.x;
                if (cmd.control0.y < y_min)
                    y_min = cmd.control0.y;
            default:
                if (cmd.to.x < x_min)
                    x_min = cmd.to.x;
                if (cmd.to.y < y_min)
                    y_min = cmd.to.y;
                break;
        }
    }

    return {x_min, y_min};
}











// -------------------------------------------------------------------------


int Path::move_to(const FT_Vector* to, void* user) {
    const auto self = static_cast<Path*>(user);
    self->path.emplace_back(
        CommandType::MOVE,
        Point{static_cast<float>(to->x) + self->current_offset.x, static_cast<float>(-to->y) - self->current_offset.y} / 64.0
    );
    return 0;
}
int Path::line_to(const FT_Vector* to, void* user) {
    const auto self = static_cast<Path*>(user);
    self->path.emplace_back(
        CommandType::LINE,
        Point{static_cast<float>(to->x) + self->current_offset.x, static_cast<float>(-to->y) - self->current_offset.y} / 64.0
    );
    return 0;
}
int Path::quad_to(const FT_Vector* control, const FT_Vector* to, void* user) {
    const auto self = static_cast<Path*>(user);
    self->path.emplace_back(
        CommandType::QUAD,
        Point{static_cast<float>(to->x) + self->current_offset.x, static_cast<float>(-to->y) - self->current_offset.y} / 64.0,
        Point{static_cast<float>(control->x) + self->current_offset.x, static_cast<float>(-control->y) - self->current_offset.y} / 64.0
    );
    return 0;
}
int Path::cubic_to(const FT_Vector* control_one, const FT_Vector* control_two, const FT_Vector* to, void* user) {
    const auto self = static_cast<Path*>(user);
    self->path.emplace_back(
        CommandType::CUBIC,
        Point{static_cast<float>(to->x) + self->current_offset.x, static_cast<float>(-to->y) - self->current_offset.y} / 64.0,
        Point{static_cast<float>(control_one->x) + self->current_offset.x, static_cast<float>(-control_one->y) - self->current_offset.y} / 64.0,
        Point{static_cast<float>(control_two->x) + self->current_offset.x, static_cast<float>(-control_two->y) - self->current_offset.y} / 64.0
    );
    return 0;
}

