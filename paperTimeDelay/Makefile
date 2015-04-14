FILES=paper.tex

all: 	paper.pdf

paper.pdf: ${FILES}
	pdflatex paper.tex
	bibtex paper
	pdflatex paper.tex
clean:
	@rm -f paper.pdf *.aux *.blg *.dvi *.log *.ps
