# def _add_vcpkg_dll_dir():
#     import platform
#     if platform.system() != 'Windows':
#         return
#     import os
#     from pathlib import Path
#     os.add_dll_directory(Path(os.environ['VCPKG_ROOT']) / 'installed' / 'x64-windows' / 'bin')


# _add_vcpkg_dll_dir()


from .path import *
from .renderer_ext import *

__all__ = ['Renderer', 'Path']