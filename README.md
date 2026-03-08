# nil - tty-based word-to-word lyrics viewer

> [!WARNING]
>
> shitty grammar and btw, some of the codes are vibecoded because, my lazy ass brain sucks

very very cool features:

- offline lyrics
- ttml parsing
- windows, linux & macos support (very cool, right?)

required niche libraries: ```libxml2```, yeah that's all

offline ttml example (`~/.config/nil/ttml.json`):

```json
[
  {
    "songname": "Do You Like Me?",
    "artist": "Daniel Caesar",
    "path": "~/MyCoolTTMLs/doyoulikeme.ttml"
  }
]
```

renderer & fetch config (`~/.config/nil/config.json`):

```json
{
  "scale": 3,
  "padding_top": 2,
  "padding_left": 8,
  "clear_screen": true,
  "main_scale": 3,
  "bg_scale": 1,
  "main_use_ascii": true,
  "show_background": true,
  "bg_requires_main": true,
  "persist_main_word": true,
  "main_row_percent": 50,
  "bg_row_from_bottom": 2,
  "main_color": "97",
  "bg_color": "2;90",
  "ascii_font": "block",
  "ascii_font_file": "~/.config/nil/fonts/custom-font.txt",
  "fetch_url_template": "https://myverycoolwebsite.com/lyrics?title={song}&artist={artist}"
}
```

colors? raw ansi codes, no fancy escaping, figure it out, mama

`ascii_font` values:

- `block` (default btw)
- `mini`
- `custom`

custom font file example:

```txt
A
 ### 
#   #
#####
#   #
#   #

QUOTE
 # # 
 # # 
   
   
   
```

cool special glyph labels: `SPACE`, `QUOTE`, `APOSTROPHE`

# installing with homebrew (macos aarch64/x86_64)

first of all, you have to fetch my repo in the command prompt:
`brew tap nurislamaibekuly/nil`
and after that, install nil:

`brew install 

# server docs

to host a server, you have to install python (idk which version)

required niche libraries: `flask`

btw you have to define `NIL_SERVER_TTML_JSON_AND_WHY_DID_I_USE_PYTHON` or, the default value would be `~/MyCoolTTMLs/ttml.json`, and yeah it's the same as the offline shit


anyways, happy listening mamas!
