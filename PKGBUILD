pkgbase=libxxc
pkgname=(libxxc libxxc-headers)
pkgver=0.1.0
pkgrel=1
pkgdesc='++C class library and libminicrt freestanding C runtime'
arch=(x86_64)
url='https://github.com/'
license=(MIT)
makedepends=(cmake ninja)

options=(staticlibs !strip !lto !debug !emptydirs)

build() {
    cmake -S "$startdir" -B "$srcdir/build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DXXC_BUILD_DEMO=OFF \
        -DXXC_BUILD_TEST=OFF
    cmake --build "$srcdir/build"
}

package_libxxc() {
    pkgdesc='++C and libminicrt shared libraries'
    DESTDIR="$pkgdir" cmake --install "$srcdir/build" --prefix /usr --component libxxc
    install -Dm644 "$startdir/LICENSE" "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}

package_libxxc-headers() {
    pkgdesc='++C and libminicrt headers, static libraries and crt0'
    depends=("libxxc=$pkgver")
    DESTDIR="$pkgdir" cmake --install "$srcdir/build" --prefix /usr --component libxxc-headers
    install -Dm644 "$startdir/LICENSE" "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
