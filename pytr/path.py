from typing import Callable, Tuple
import renderer

class Path(renderer.Path):
    def string(self) -> str:
        return super().string()
    
    def as_rel(self) -> 'Path':
        return super().as_rel()
    
    def transform(self, tr: Callable[[float, float], Tuple[float, float]]) -> 'Path':
        return super().transform(tr)

    def to_cubic(self) -> 'Path':
        return super().to_cubic()

    def reorder(self) -> 'Path':
        return super().reorder()
