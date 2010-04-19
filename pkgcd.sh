pkgcd() {
	local repo pkg

	if [ -z $1 ]; then
		echo "Usage: pkgcd <package>"
		echo "Usage: pkgcd <repo> <package>"
		return
	fi

	if [ -z $2 ]; then
		repo=current
		pkg=$1
	else
		repo=$1
		pkg=$2
	fi

	source ~/.repoman.conf

	[ $? -ne 0 ] && return

	if [ -z $fst_root ]; then
		echo "fst_root from $HOME/.repoman.conf is not defined."
		return
	fi

	cd $fst_root/$repo/source/*/$pkg
}
