<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN"[
<!ENTITY dbstyle SYSTEM "/usr/share/sgml/docbook/stylesheet/dsssl/modular/html/docbook.dsl" CDATA DSSSL>
]>

<style-sheet>
<style-specification use="docbook">
<style-specification-body>


(define ($html-body-start$)
  (make sequence
      (make element gi: "table"
	    attributes: (list (list "align" "center")
			      (list "width" "100%")
			      (list "bgcolor" "0000a5"))
	    (make element gi: "tr"
		  attributes: (list (list "align" "center"))
		  (make element gi: "td"
			(make element gi: "table"
			      attributes: (list (list "align" "center")
						(list "width" "100%"))
			      (make element gi: "tr"
				    (make sequence

				    (make element gi: "td"
					  (make element gi: "center"
						(make element gi: "b"
						      (make element gi: "a"
							    attributes: (list (list "href" "../index.shtml"))
							    (make element gi: "font"
								  attributes: (list (list "color" "ffff55"))
								  (literal "Main")
								  ))))
					  )
				    (make element gi: "td"
					  (make element gi: "center"
						(make element gi: "b"
						      (make element gi: "a"
							    attributes: (list (list "href" "../links.html"))
							    (make element gi: "font"
								  attributes: (list (list "color" "ffff55"))
								  (literal "Links")
								  ))))
					  )
				    (make element gi: "td"
					  (make element gi: "center"
						(make element gi: "b"
						      (make element gi: "a"
							    attributes: (list (list "href" "../games.html"))
							    (make element gi: "font"
								  attributes: (list (list "color" "ffff55"))
								  (literal "Games")
								  ))))
					  )
				    (make element gi: "td"
					  (make element gi: "center"
						(make element gi: "b"
						      (make element gi: "a"
							    attributes: (list (list "href" "../faq.html"))
							    (make element gi: "FAQ"
								  attributes: (list (list "color" "ffff55"))
								  (literal "Main")
								  ))))
					  )
				    (make element gi: "td"
					  (make element gi: "center"
						(make element gi: "b"
						      (make element gi: "a"
							    attributes: (list (list "href" "../download.html"))
							    (make element gi: "font"
								  attributes: (list (list "color" "ffff55"))
								  (literal "Download")
								  ))))
					  )
				    (make element gi: "td"
					  (make element gi: "center"
						(make element gi: "b"
						      (make element gi: "a"
							    attributes: (list (list "href" "../docs.html"))
							    (make element gi: "font"
								  attributes: (list (list "color" "ffff55"))
								  (literal "Documentation")
								  ))))
					  )
				    (make element gi: "td"
					  (make element gi: "center"
						(make element gi: "b"
						      (make element gi: "a"
							    attributes: (list (list "href" "../devel.html"))
							    (make element gi: "font"
								  attributes: (list (list "color" "ffff55"))
								  (literal "Development")
								  ))))
					  )
				    (make element gi: "td"
					  (make element gi: "center"
						(make element gi: "b"
						      (make element gi: "a"
							    attributes: (list (list "href" "http://lag.net/freesci/bugmgr.cgi?mode=open"))
							    (make element gi: "font"
								  attributes: (list (list "color" "ffff55"))
								  (literal "Bug list")
								  ))))
					  )
				    )
				    )))))
      (make empty-element gi: "hr")
      (empty-sosofo)
)
)

(define %admon-graphics%
  ;; REFENTRY admon-graphics
  ;; PURP Use graphics in admonitions?
  ;; DESC
  ;; If true, admonitions are presented in an alternate style that uses
  ;; a graphic.  Default graphics are provided in the distribution.
  ;; /DESC
  ;; AUTHOR N/A
  ;; /REFENTRY
  #t)

(define %admon-graphics-path%
  ;; REFENTRY admon-graphics-path
  ;; PURP Path to admonition graphics
  ;; DESC
  ;; Sets the path, probably relative to the directory where the HTML
  ;; files are created, to the admonition graphics.
  ;; /DESC
  ;; AUTHOR N/A
  ;; /REFENTRY
  "../images/")

(define ($admon-graphic$ #!optional (nd (current-node)))
  ;; REFENTRY admon-graphic
  ;; PURP Admonition graphic file
  ;; DESC
  ;; Given an admonition node, returns the name of the graphic that should
  ;; be used for that admonition.
  ;; /DESC
  ;; AUTHOR N/A
  ;; /REFENTRY
  (cond ((equal? (gi nd) (normalize "tip"))
	 (string-append %admon-graphics-path% "tip.png"))
	((equal? (gi nd) (normalize "note"))
	 (string-append %admon-graphics-path% "note.png"))
	((equal? (gi nd) (normalize "important"))
	 (string-append %admon-graphics-path% "important.png"))
	((equal? (gi nd) (normalize "caution"))
	 (string-append %admon-graphics-path% "caution.png"))
	((equal? (gi nd) (normalize "warning"))
	 (string-append %admon-graphics-path% "warning.png"))
	(else (error (string-append (gi nd) " is not an admonition.")))))


(define %body-attr% 
  ;; REFENTRY body-attr
  ;; PURP What attributes should be hung off of BODY?
  ;; DESC
  ;; A list of the the BODY attributes that should be generated.
  ;; The format is a list of lists, each interior list contains the
  ;; name and value of a BODY attribute.
  ;; /DESC
  ;; AUTHOR N/A
  ;; /REFENTRY
  (list
   (list "BGCOLOR" "#F0F0F0")
   (list "TEXT" "#000000")
   (list "LINK" "#0000FF")
   (list "VLINK" "#840084")
   (list "ALINK" "#0000FF")))



<![%html;[

]]>



</style-specification-body>
</style-specification>
<external-specification id="docbook" document="dbstyle">
</style-sheet>

