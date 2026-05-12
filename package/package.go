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
	// dependencies
	cunittestpkg := cunittest.GetPackage()
	corepkg := rcore.GetPackage()

	// main package
	mainpkg := denv.NewPackage(repo_path, repo_name)
	mainpkg.AddPackage(corepkg)
	mainpkg.AddPackage(cunittestpkg)

	// DWO library
	dwoLib := denv.SetupCppLibraryForArduinoEsp32(mainpkg, "lib_dwo", "dwo")
	dwoLib.AddDependencies(corepkg.GetMainLib())

	// WCS library
	wcsLib := denv.SetupCppLibraryForArduinoEsp32(mainpkg, "lib_wcs", "wcs")
	wcsLib.AddDependencies(corepkg.GetMainLib())

	mainpkg.AddLibrary(dwoLib)
	mainpkg.AddLibrary(wcsLib)

	return mainpkg
}
