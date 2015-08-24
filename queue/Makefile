FILES=queuing.tex

all: 	queuing.pdf

queuing.pdf: ${FILES}
	pdflatex queuing.tex
	bibtex queuing
	pdflatex queuing.tex
clean:
	@rm -f queuing.pdf *.aux *.blg *.dvi *.log *.ps
