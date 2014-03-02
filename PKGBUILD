# $Id$
# Maintainer: Youn-sok Choi <digitie@gmail.com>

pkgbase=python2-atasmart
_pkgname=pyatasmart
pkgname=$pkgbase-git
pkgver=0.0.1
pkgrel=1
pkgdesc='Simple Python binding of libatasmart'
license=('GPL2')
arch=('x86_64' 'i686' 'arm')
url='http://github.com/digitie/pyatasmart'
makedepends=('python2-setuptools')
depends=('libatasmart>=0.19')
source=('git://github.com/digitie/pyatasmart.git')
md5sums=('SKIP')

pkgver() {
    cd "$srcdir/$_pkgname"
    echo $(git rev-list --count HEAD).$(git rev-parse --short HEAD)
}


build() {
    cd "$srcdir/$_pkgname"

    python setup.py build
}

package() {
    cd "$srcdir/$_pkgname"

    python setup.py install --root=${pkgdir} --optimize=1

    install -D -m644 LICENSE ${pkgdir}/usr/share/licenses/python2-atasmart
}

# vim:set ts=2 sw=2 et:
