(TeX-add-style-hook "main"
 (lambda ()
    (TeX-add-symbols
     '("mat" 1)
     '("bvec" 1))
    (TeX-run-style-hooks
     "subfig"
     "multirow"
     "dcolumn"
     "array"
     "inputenc"
     "ansinew"
     "latex2e"
     "ACIN_bac_report10"
     "ACIN_bac_report"
     "NoListOfFigures"
     "NoListOfTables"
     "ngerman"
     "cha1_TemplateIntroduction")))

