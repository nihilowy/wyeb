
export DEBUG=1
make re > /dev/null 2>&1

if test -e wyeb; then
	./wyeb
else
	export DEBUG=0
	#show error
	make
fi

make clean
