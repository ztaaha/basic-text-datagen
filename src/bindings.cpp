#include <pybind11/pybind11.h>
#include <pybind11/eigen/tensor.h>
#include <pybind11/stl.h>
#include <fmt/format.h>

#include "render.h"
#include "path.h"

namespace py = pybind11;



PYBIND11_MODULE(renderer, m) {

    py::class_<Renderer>(m, "Renderer")
        .def(py::init<>())
        .def("set_font", &Renderer::set_font)
        .def("set_text", &Renderer::set_text)
        .def("text_paths", &Renderer::text_paths)
        .def("render_text", [](Renderer& r, const unsigned font_size, const std::string& mode) {
            if (mode == "freetype")
                return r.render_text(font_size, RenderMode::FREETYPE);
            if (mode == "skia")
                return r.render_text(font_size, RenderMode::SKIA);

            throw std::invalid_argument(fmt::format("Mode {} is not recognized", mode));
        })
        .def("cluster_strings", &Renderer::cluster_strings);


    py::class_<Path>(m, "Path")
        .def("string", &Path::string)
        .def("as_rel", &Path::as_rel)
        .def("to_cubic", &Path::to_cubic)
        .def("transform", &Path::transform)
        .def("reorder", &Path::reorder)
        .def_property_readonly("first_x", [](const Path& p) { return p.get_commands().front().to.x; });


}