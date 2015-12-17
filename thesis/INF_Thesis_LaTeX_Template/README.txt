
Vienna University of Technology - Faculty of Informatics
TUINF thesis - Readme

For questions and comments send an email to
Thomas Auzinger <thomas.auzinger@cg.tuwien.ac.at>
Thomas Krennwallner <tkren@kr.tuwien.ac.at>

---------------------------------------------------------

TU Wien - Fakultät für Informatik
TUINF Arbeit - Readme

Für Fragen oder Kommentare schicken Sie eine Email an
Thomas Auzinger <thomas.auzinger@cg.tuwien.ac.at>
Thomas Krennwallner <tkren@kr.tuwien.ac.at>



DESCRIPTION

This is a LaTeX template for a thesis at the Faculty of Informatics of the Vienna
University of Technology.

FILES

thesis.tex                      The main file in which all \thesis___ definitions have to be set.
thesis.pdf                      The intended output of a pdflatex compilation of thesis.tex.
TUINFBA.sty                     The latex style file of the thesis template.
bibgerm.sty,                    Non-standard style files that are used for the template. These can
  geometry.sty,                   be replaced by more current versions.
  ngerman.sty                 
figures/TU_INF_header.*,        The graphical elements of the template
  figures/TU_INF_Logo_gray.*    
chapters/*.tex                  The chapters of thesis.tex. introduction.tex, design.tex and
                                  bibliographic.tex can be deleted and replaced by the content of
                                  the thesis. All other files have to be edited.
chapters/references.bib         The references in BibTeX format that are used in the generation of
                                  thesis.pdf. The content of this file has to be replaced by the
                                  references of the thesis.

USE

1.) Edit and compile chapters/references.bib with BibTeX.
2.) Create or edit chapters in chapters/*.tex.
3.) Define all \thesis___ definitions in thesis.tex, add all chapters, and compile with LaTeX.

---------------------------------------------------------



BESCHREIBUNG

Dies ist ein LaTeX Template für eine Arbeit an der Fakultät für Informatik an der
Technischen Universität Wien.

DATEIEN

thesis.tex                      Die Haupdatei in der all \thesis___ Definitionen gesetzt werden
                                  müssen.
thesis.pdf                      Die erwartete Ausgabe einer pdflatex Kompilation von thesis.tex.
TUINFBA.sty                     Die LaTeX style Datei des Arbeit Templates.
bibgerm.sty,                    Zusätzliche style Dateien, die in diesem Template verwendet werden.
  geometry.sty,                   Diese können durch neuere Versionen ersetzt werden.
  ngerman.sty                 
figures/TU_INF_header.*,        Die grafischen Elemente des Templates.
  figures/TU_INF_Logo_gray.*    
chapters/*.tex                  Die Kapitel von thesis.tex. introduction.tex, design.tex und
                                  bibliographic.tex können gelöscht werden und durch den Inhalt der
                                  Arbeit ersetzt werden. Alle anderen Dateien müssen editiert
                                  werden.
chapters/references.bib         Die Referenzen im BibTeX format, die in der Erstellung von
                                  thesis.pdf verwendet werden. Der Inhalt dieser Datei muss durch
                                  die Referenzen der Arbeit ersetzt werden.

ANWENDUNG

1.) Editiere und kompiliere chapters/erferences.bib mit BibTeX.
2.) Erstelle oder editiere die Kapitel in chapters/*.tex
3.) Definiere alle \thesis___ Definitionen in thesis.tex, füge alle Kapitel hinzu und kompiliere
      thesis.tex mit LaTeX.