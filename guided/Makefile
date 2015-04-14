FILES=guided.tex

all: 	guided.pdf

guided.pdf: ${FILES}
	pdflatex guided.tex
	bibtex guided
	pdflatex guided.tex
	pdflatex guided.tex
clean:
	@rm -f guided.pdf *.aux *.blg *.dvi *.log *.ps
