package rlcd

import (
	denv "github.com/jurgen-kluft/ccode/denv"
	cunittest "github.com/jurgen-kluft/cunittest/package"
	rcore "github.com/jurgen-kluft/rcore/package"
)

const (
	repo_path = "github.com\\jurgen-kluft"
	repo_name = "rlcd"
)

func GetPackage() *denv.Package {
	name := repo_name

	// dependencies
	cunittestpkg := cunittest.GetPackage()
	corepkg := rcore.GetPackage()

	// main package
	mainpkg := denv.NewPackage(repo_path, repo_name)
	mainpkg.AddPackage(corepkg)
	mainpkg.AddPackage(cunittestpkg)

	// TODO, make a library per sensor ?

	// main library
	mainLib := denv.SetupCppLibProject(mainpkg, name)
	mainLib.AddDependencies(corepkg.GetMainLib())

	// test library
	mainTestLib := denv.SetupCppTestLibProject(mainpkg, name)
	mainTestLib.AddDependencies(corepkg.GetTestLib())

	// DWO library
	dwoLib := denv.SetupCppLibraryForArduinoEsp32(mainpkg, "lib_dwo", "dwo")
	dwoLib.AddDependencies(corepkg.GetMainLib())

	// WCS library
	wcsLib := denv.SetupCppLibraryForArduinoEsp32(mainpkg, "lib_wcs", "wcs")
	wcsLib.AddDependencies(corepkg.GetMainLib())

	// unittest project
	maintest := denv.SetupCppTestProject(mainpkg, name)
	maintest.AddDependencies(cunittestpkg.GetMainLib())
	maintest.AddDependency(mainTestLib)

	mainpkg.AddMainLib(mainLib)
	mainpkg.AddLibrary(dwoLib)
	mainpkg.AddLibrary(wcsLib)

	mainpkg.AddTestLib(mainTestLib)
	mainpkg.AddUnittest(maintest)

	return mainpkg
}
