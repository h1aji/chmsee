# chmsee pathced to work with webkit
# contributor: murad ustarkhanov - hm dot ust at live dot com

pkgname=chmsee-git
pkgbase=chmsee
pkgver=1.4.0
pkgrel=1
arch=('i686' 'x86_64')
pkgdesc="A chm (MS HTML help file format) viewer based on webkit."
url="http://chmsee.googlecode.com/"
license="GPL"
depends=('gtk2' 'libglade' 'chmlib' 'webkitgtk2')
makedepends=('intltool' 'cmake')
provides=('chmsee')
conflicts=('chmsee')
source=(https://chmsee.googlecode.com/files/$pkgbase-$pkgver.tar.gz)
md5sums=('7c226e6f518284a040ad3b9433d5e218')

build() {

  cd ${srcdir}/$pkgbase-$pkgver

  mkdir build
  cd build

  cmake -DCMAKE_INSTALL_PREFIX=/usr ..
  make
}

package() {
  cd ${srcdir}/$pkgbase-$pkgver/build

  make DESTDIR="$pkgdir" install
}

