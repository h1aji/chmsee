# chmsee webkit version
# contributor: murad ustarkhanov - hm dot ust at live dot com

_pkgname=chmsee
pkgname=${_pkgname}-git
pkgver=1.5.0
pkgrel=1
arch=('i686' 'x86_64')
pkgdesc="A chm (MS HTML help file format) viewer based on webkit."
url="https://h1aji.github.io/chmsee"
license="GPL"
depends=('gtk3' 'chmlib' 'webkitgtk')
makedepends=('intltool' 'cmake' 'libgcrypt' 'libxml2')
provides=('chmsee')
conflicts=('chmsee')
source=("${_pkgname}"::git+https://github.com/h1aji/chmsee.git)
sha256sums=('SKIP')

build() {

  cd ${srcdir}/${_pkgname}

  mkdir build
  cd build

  cmake -DCMAKE_INSTALL_PREFIX=/usr ..
  make
}

package() {
  cd ${srcdir}/${_pkgname}/build

  make DESTDIR="$pkgdir" install
}

