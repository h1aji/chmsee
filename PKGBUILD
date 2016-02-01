# chmsee webkit version
# contributor: murad ustarkhanov - hm dot ust at live dot com

pkgname=chmsee
pkgver=1.4.0
pkgrel=1
arch=('i686' 'x86_64')
pkgdesc="A chm (MS HTML help file format) viewer based on webkit."
url="https://github.com/h1aji/chmsee"
license="GPL"
depends=('gtk3' 'chmlib' 'webkitgtk')
makedepends=('intltool' 'cmake' 'libgcrypt')
provides=('chmsee')
conflicts=('chmsee')
source=(https://github.com/h1aji/$pkgname/archive/$pkgname-$pkgver.tar.gz)
md5sums=('997c1f03fcc1ec3e4306db3ca2295617')

build() {

  cd ${srcdir}/$pkgname-$pkgver

  mkdir build
  cd build

  cmake -DCMAKE_INSTALL_PREFIX=/usr ..
  make
}

package() {
  cd ${srcdir}/$pkgname-$pkgver/build

  make DESTDIR="$pkgdir" install
}

