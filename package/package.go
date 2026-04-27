package rlcd

import (
	denv "github.com/jurgen-kluft/ccode/denv"
	rcore "github.com/jurgen-kluft/rcore/package"
)

const (
	repo_path = "github.com\\jurgen-kluft"
	repo_name = "rlcd"
)

func GetPackage() *denv.Package {
	name := repo_name

	// dependencies
	corepkg := rcore.GetPackage()

	// TODO make a library per LCD type

	// main package
	mainpkg := denv.NewPackage(repo_path, repo_name)
	mainpkg.AddPackage(corepkg)

	// main library
	mainlib := denv.SetupCppLibProject(mainpkg, name)
	mainlib.AddDependencies(corepkg.GetMainLib())

	mainpkg.AddMainLib(mainlib)
	return mainpkg
}
