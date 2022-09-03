all: tests.html tests.pdf

tests.html: tests.md
	pandoc -f markdown+smart+pipe_tables+backtick_code_blocks -t html5 --standalone tests.md > tests.html

tests.pdf: tests.md
	pandoc -f markdown+smart+pipe_tables+backtick_code_blocks -t pdf -o tests.pdf --standalone -V fontenc=T2A tests.md	

view:
	links -G tests.html

clean:
	rm tests.html
