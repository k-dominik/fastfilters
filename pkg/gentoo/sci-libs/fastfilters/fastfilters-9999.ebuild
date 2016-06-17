# Copyright 1999-2014 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Id$

EAPI=6
PYTHON_COMPAT=( python2_7 python3_4 )
PYTHON_REQ_USE="threads,xml"

DESCRIPTION="fast gaussian and derivative convolutional filters"
HOMEPAGE="https://github.com/svenpeter42/fastfilters"

if [[ $PV = *9999* ]]; then
	EGIT_BRANCH="devel"
else
	EGIT_COMMIT="v${PV}"
fi

EGIT_REPO_URI="https://github.com/svenpeter42/fastfilters.git"

inherit flag-o-matic python-r1 python-utils-r1 git-r3 cmake-utils

LICENSE="MIT"
SLOT="0"
KEYWORDS="~amd64"
IUSE=""

RDEPEND="
	${PYTHON_DEPS}
	dev-python/numpy[${PYTHON_USEDEP}]
	"

DEPEND="${RDEPEND}"

REQUIRED_USE=""

DOCS=( README.md )

pkg_setup() {
	python_setup
}

src_configure() {
	fastfilters_configure() {
		filter-flags -mavx -mfma -mavx2 -march=* -mtune=*
		cmake-utils_src_configure
	}
	python_setup
	python_foreach_impl fastfilters_configure
}

src_compile() {
	fastfilters_compile() {
		cmake-utils_src_compile
	}
	python_foreach_impl fastfilters_compile
}

src_install() {
	fastfilters_install() {
		cmake-utils_src_install
	}
	python_foreach_impl fastfilters_install
}
