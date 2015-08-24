FILES=sandbox-techreport.tex

all: 	sandbox-techreport.pdf

sandbox-techreport.pdf: ${FILES}
	pdflatex sandbox-techreport.tex
	bibtex sandbox-techreport
	pdflatex sandbox-techreport.tex
	pdflatex sandbox-techreport.tex
clean:
	@rm -f sandbox-techreport.pdf sandbox-techreport.bbl *.aux *.blg *.dvi *.log *.ps
	@rm -f sandbox-techreport.ps.gz sandbox.pdf
