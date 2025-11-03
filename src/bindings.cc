
#include <pybind11/pybind11.h>
#include <pybind11/eigen/tensor.h>
#include <pybind11/stl.h>
#include <fmt/format.h>
#include <optional>
#include <utility>

#include "render.h"
#include "path.h"

namespace py = pybind11;
using namespace pybind11::literals;



PYBIND11_MODULE(renderer, m) {

    py::class_<Renderer>(m, "Renderer")
        .def(py::init<>())
        .def("set_font", &Renderer::set_font)
        .def("set_text", &Renderer::set_text)
        .def("text_paths", &Renderer::text_paths)
        .def("render_text", [](Renderer& r, const unsigned font_size, const std::string& mode, std::optional<std::string> myfonts_id) {
            RenderMode m;
            if (mode == "freetype") {
                m = RenderMode::FREETYPE;
            } else if (mode == "myfonts") {
                m = RenderMode::MYFONTS;
            } else {
                throw std::invalid_argument("Invalid render mode: " + mode);
            }
            return r.render_text(font_size, m, std::move(myfonts_id));
        }, "font_size"_a, "mode"_a, "myfonts_id"_a = py::none())
        .def("cluster_strings", &Renderer::cluster_strings)
        .def("shape_if_needed", &Renderer::shape_if_needed)
        .def("shape", &Renderer::shape)
        .def("max_advance", &Renderer::max_advance)
        .def("cluster_windows", &Renderer::cluster_windows);


    py::class_<Path>(m, "Path")
        .def("string", &Path::string)
        .def("as_rel", &Path::as_rel)
        .def("to_cubic", &Path::to_cubic)
        .def("transform", &Path::transform)
        .def("reorder", &Path::reorder)
        .def_property_readonly("first_x", [](const Path& p) { return p.get_commands().front().to.x; });


    py::class_<ClusterWindow>(m, "ClusterWindow")
        .def(py::init<>())
        .def_readwrite("x", &ClusterWindow::x)
        .def_readwrite("end", &ClusterWindow::end);

}