import urllib.parse
import numpy as np
import requests
from PIL import Image

def make_preview_url(ident, text, size, width=4000, spacing=0, fg_color="000000", bg_color="FFFFFF", scale=1, lang="en"):
    text = urllib.parse.quote_plus(text)
    return f"https://sig.monotype.com/render/105/font/{ident}?rt={text}&rs={size}&w={width}&fg={fg_color}&bg={bg_color}&t=o&sc={scale}&userLang={lang}&render_mode=new&tr={spacing}"


def trim_img(img, white_bg=False):
    bg = 255 if white_bg else 0
    if img.ndim == 2:
        nz = np.where(img != bg)
    else:
        nz = np.where(img[0] != bg)
    return img[..., np.min(nz[0]):np.max(nz[0]) + 1, np.min(nz[1]):np.max(nz[1]) + 1]

