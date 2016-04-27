CROSS_COMPONENTS_LINKS=(ftp://ftp.gnu.org/gnu/binutils/binutils-2.26.tar.gz \
				https://gmplib.org/download/gmp/gmp-6.1.0.tar.xz \
				http://www.mpfr.org/mpfr-current/mpfr-3.1.4.tar.gz \
				http://www.bastoul.net/cloog/pages/download/count.php3?url=./cloog-0.18.4.tar.gz \
				ftp://ftp.gnu.org/gnu/mpc/mpc-1.0.3.tar.gz \
				http://ftp.gnu.org/gnu/texinfo/texinfo-6.1.tar.gz \
				ftp://ftp.gnu.org/gnu/gcc/gcc-4.9.3/gcc-4.9.3.tar.gz)
mkdir build
cd build
for i in "${CROSS_COMPONENTS_LINKS[@]}"
do
	wget $i 
done

for i in `ls *.gz`
do
	echo $i
	tar -xzf $i 
done

for i in `ls *.xz`
do
	echo $i
	tar -xJf $i 
done

export PREFIX="$HOME/cross2/"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
cd gcc-4.9.3
ln -s ../mpfr-3.1.4 mpfr
ln -s ../gmp-6.1.0 gmp
ln -s ../mpc-1.0.3 mpc
#ln -s ../isl-0.12.2 isl
ln -s ../cloog-0.18.4 cloog
cd ..

mkdir build-binutils
cd build-binutils
../binutils-2.26/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install
cd ..

mkdir build-gcc
cd build-gcc
../gcc-4.9.3/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
cd ..
cd ..
rm -rf build/
