pkgbase=libxxc
pkgname=(libxxc libxxc-headers)
pkgver=0.1.1
pkgrel=1
pkgdesc='++C class library and libminicrt freestanding C runtime'
arch=(x86_64)
url='https://github.com/DanielLMcGuire/plusplusC'
license=(MIT)
makedepends=(make gcc)

options=(staticlibs !strip !lto !debug !emptydirs)

_make() {
    make -C "$startdir" BUILD="$srcdir/build" PREFIX=/usr CFLAGS= LTO=1 "$@"
}

build() {
    _make -j"$(nproc)" libs shared
}

package_libxxc() {
    pkgdesc='++C and libminicrt shared libraries'
    _make DESTDIR="$pkgdir" install-libs
    install -Dm644 "$startdir/LICENSE" "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}

package_libxxc-headers() {
    pkgdesc='++C and libminicrt headers, static libraries and crt0'
    depends=("libxxc=$pkgver")
    _make DESTDIR="$pkgdir" install-dev
    install -Dm644 "$startdir/LICENSE" "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
