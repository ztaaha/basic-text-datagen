import renderer
from playwright.sync_api import sync_playwright
import numpy as np
from PIL import Image
from io import BytesIO
from typing import Literal
from numpy.typing import NDArray
from uuid import uuid4
import os
from path import Path



ADD_SPANS = """
([fontPath, fontName, size, strings]) => {
    document.querySelector('div').remove();


    const div = document.createElement('div');
    div.style.display = 'inline-block';
    div.style.fontSize = `${size}px`;

    const font = new FontFace(fontName, `url(file:///${fontPath})`);

    return font.load().then(() => {
        document.fonts.add(font);
        div.style.fontFamily = fontName;
        
        for (const str of strings) {
            const span = document.createElement('span');
            span.textContent = str;
            div.appendChild(span);
        }
        document.body.appendChild(div);
    });
}
"""


class Renderer(renderer.Renderer):
    def __init__(self):
        super().__init__()


    def start_web(self) -> 'Renderer':
        self._init_browser()
        return self
    
    def _init_browser(self):
        base_path = os.path.abspath('base.html').replace('\\', '/')
        if not os.path.isfile(base_path):
            raise FileNotFoundError(f"base.html file doesn't exist")
    
        self._playwright = sync_playwright().start()
        for name in ['chromium', 'firefox']:
            browser = getattr(self._playwright, name).launch()
            page = browser.new_page(viewport={'width': 10000, 'height': 10000})
            page.goto(f"file:///{base_path}")
            setattr(self, f"_{name}", browser)
            setattr(self, f"_page_{name}", page)
    
    def end_web(self):
        for name in ['chromium', 'firefox']:
            if hasattr(self, f'_{name}'):
                getattr(self, f'_{name}').close()
        self._playwright.stop()

    def __enter__(self) -> 'Renderer':
        return self.start_web()
    
    def __exit__(self, *args):
        self.end_web()


    def set_font(self, font_path: str):
        self.font_path = os.path.abspath(font_path).replace('\\', '/')
        super().set_font(self.font_path)

    def set_text(self, text: str):
        super().set_text(text)

    def text_paths(self) -> tuple[list[Path], list[float]]:
        return super().text_paths()


    def render_text(
                self, 
                size: int, 
                mode: Literal['freetype', 'chromium', 'firefox', 'skia']
            ) -> NDArray[np.uint8]:
        """Render text with size and mode
            
            Image and masks as (I, H, W). I = 0 is image, I > 0 are cluster masks.
        """

        if mode == 'freetype' or mode == 'skia':
            imgs =  super().render_text(size, mode)
        elif mode == 'chromium' or mode == 'firefox':
            imgs = self._web_render_text(size, mode)
        else:
            raise ValueError(f"Mode \"{mode}\" doesn't exist")
        
        nz = np.where(imgs[0] != 255)
        return imgs[:, min(nz[0]):max(nz[0])+1, min(nz[1]):max(nz[1])+1]





    def _web_render_text(self, size, mode):
        assert hasattr(self, f'_page_{mode}'), f"{mode} browser not included in renderer initialization"
        page = getattr(self, f'_page_{mode}')

        strings = super().cluster_strings()
        id = "font-" + str(uuid4())
        page.evaluate(ADD_SPANS, [self.font_path, id, size, strings])
        div_locator = page.locator('div')

        buf = div_locator.screenshot()
        imgs = [np.array(Image.open(BytesIO(buf)).convert('L'))]

        page.evaluate("() => { document.querySelectorAll('span').forEach(span => { span.style.opacity = 0; }); }")

        for i in range(len(strings)):
            curr_span = page.locator('span').nth(i)
            curr_span.evaluate("cluster => { cluster.style.opacity = 1; }")
            buf = div_locator.screenshot()
            mask = np.array(Image.open(BytesIO(buf)).convert('L'))
            mask = np.where(mask != 255, 1, 0)
            imgs.append(mask)
            curr_span.evaluate("cluster => { cluster.style.opacity = 0; }")
            
        return np.stack(imgs, axis=0)





