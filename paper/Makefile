FILES=sandbox.tex

all: sandbox.pdf

sandbox.pdf: ${FILES}
	latex sandbox.tex
	bibtex sandbox
	latex sandbox.tex
	latex sandbox.tex
	dvips -o sandbox.ps -t Letter -Ppdf -G0 sandbox.dvi
	ps2pdf sandbox.ps sandbox.pdf

ps: sandbox.ps.gz
	@ggv sandbox.pdf &

sandbox.ps.gz: ${FILES}
	@latex sandbox.tex
	@bibtex sandbox
	@latex sandbox.tex
	@latex sandbox.tex
	@dvips -o sandbox.ps -t Letter -Pcmz -G0 sandbox.dvi
	@ps2pdf sandbox.ps sandbox.pdf
	@gzip -9 -f sandbox.ps

pdf: sandbox.pdf
	@ggv sandbox.pdf &

clean:
	@rm -f sandbox.pdf sandbox.bbl *.aux *.blg *.dvi *.log *.ps
	@rm -f sandbox.ps.gz sandbox.pdf
