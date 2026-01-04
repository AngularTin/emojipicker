Can be compilled with the following command
```bash
g++ -std=c++17 emojipicker.cpp -o emojipicker $(fltk-config --cflags --ldflags)
```
It works well with the following wrapper, which can be bound to e.g. MOD + "."

```bash
#!/bin/bash

# Launches the picker (blocking)
~<Path to the compiled project>

# Waits a tiny bit to make sure clipboard is ready
sleep 0.05

# Pastes the clipboard into the current window
# Requires xdotool installed
xdotool type --clearmodifiers "$(xclip -o -selection clipboard)"
```

