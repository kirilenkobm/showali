# Showali – TUI alignment viewer

![Made with Cursor](https://img.shields.io/badge/Made%20with-Cursor-0066cc?style=flat&logo=cursor&logoColor=white)

Sometimes you just want to run `showali my_alignment.fasta` and see your sequences.

![Screenshot](pics/v0.1.7.screenshot.png)
*v0.1.5 screenshot*

## How to install

```bash
git clone git@github.com:kirilenkobm/showali.git
cd showali
make
# add bin/showali to your path, enjoy!
```

## Future Plans

1. **Additional formats support & auto-detect**  
   Add parsers for `.aln`, `.phy` and `.maf` (and others) with automatic format detection.

2. **`--no-color` option**  
   Disable ANSI color codes (for scripting and clean screenshots).

3. **WASD navigation (fast scroll)**  
   Jump by half-screen using WASD.

4. **Block select**  
   Rectangular selection of nucleotides for easy copying.

5. **`S` (Search) command (?)**  
   In-terminal pattern search with result navigation and history, keeping the UI minimal.

## That's pretty much it.
