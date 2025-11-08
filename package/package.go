package rdno_lcd

import (
	denv "github.com/jurgen-kluft/ccode/denv"
	rdno_core "github.com/jurgen-kluft/rdno_core/package"
)

const (
	repo_path = "github.com\\jurgen-kluft"
	repo_name = "rdno_lcd"
)

func GetPackage() *denv.Package {
	name := repo_name

	// dependencies
	corepkg := rdno_core.GetPackage()

	// main package
	mainpkg := denv.NewPackage(repo_path, repo_name)
	mainpkg.AddPackage(corepkg)

	// main library
	mainlib := denv.SetupCppLibProject(mainpkg, name)
	mainlib.AddDependencies(corepkg.GetMainLib())

	mainpkg.AddMainLib(mainlib)
	return mainpkg
}
