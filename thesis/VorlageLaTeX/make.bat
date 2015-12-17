latex.exe -quiet -interaction=nonstopmode --src-specials main.tex
bibtex.exe --quiet main
latex.exe -quiet -interaction=nonstopmode --src-specials main.tex
latex.exe -quiet -interaction=nonstopmode --src-specials main.tex
dvips.exe -quiet -Ppdf -ta4 -R0 -o main.ps main.dvi
ps2pdf.exe -r600 -dBATCH -dNOPAUSE -q  main.ps main.pdf

start main.pdf
pause