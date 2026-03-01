# Instruction Manual Template

This directory contains a professional LaTeX template for writing technical engineering models' assembly instructions and manuals.

## Project Structure
- `main.tex`: The main document. It contains the preamble with configuration, custom environments for notices and steps, and an example document structure.
- `images/`: Put your photos, CAD renders, or illustrations in this directory.

## Features
- **Custom Callout Boxes**: Includes environments like `\begin{warningbox}`, `\begin{notebox}`, and `\begin{tipbox}`.
- **Assembly Steps**: Features a clean `\begin{assemblystep}{Title}` environment that auto-increments and formats steps identically.
- **Clean BOM Table**: A pre-formatted Bill of Materials with checkboxes.
- **Two-Column Step Layout**: Demonstrates how to write step text on one side and showcase CAD/reference images on the other using `minipage`. 

## How to Compile
You can compile this either using an online LaTeX editor overleaf, or locally if you have installed a TeX distribution (TeX Live / MiKTeX).

**Command Line Build:**
```bash
pdflatex main.tex
# Or run toolchains like latexmk
latexmk -pdf main.tex
```

## Tips for Images
When adding an image, uncomment the placeholder `{\color{gray}\rule...}` blocks and add:
```latex
\includegraphics[width=\linewidth]{images/my-image.png}
```
You can use photos (.jpg/.png) or exported CAD drawings (.png, .pdf). 
