package rlcd

import (
	denv "github.com/jurgen-kluft/ccode/denv"
	cunittest "github.com/jurgen-kluft/cunittest/package"
	rcore "github.com/jurgen-kluft/rcore/package"
	respressif_components "github.com/jurgen-kluft/respressif-components/package"
)

const (
	repo_path = "github.com\\jurgen-kluft"
	repo_name = "rlcd"
)

func GetPackage() *denv.Package {
	// dependencies
	cunittestpkg := cunittest.GetPackage()
	corepkg := rcore.GetPackage()
	respressif_componentspkg := respressif_components.GetPackage()

	// main package
	mainpkg := denv.NewPackage(repo_path, repo_name)
	mainpkg.AddPackage(corepkg)
	mainpkg.AddPackage(cunittestpkg)
	mainpkg.AddPackage(respressif_componentspkg)

	// DWO library
	dwoLib := denv.SetupCppLibraryForArduinoEsp32(mainpkg, "lib_dwo", "dwo")
	dwoLib.AddDependencies(corepkg.GetMainLib())

	// WCS library
	wcsLib := denv.SetupCppLibraryForArduinoEsp32(mainpkg, "lib_wcs", "wcs")
	wcsLib.AddDependencies(corepkg.GetMainLib())

	// GUITION library
	guitionLib := denv.SetupCppLibraryForArduinoEsp32(mainpkg, "lib_guition", "guition")
	guitionLib.AddDependency(respressif_componentspkg.GetLibrary("library_esp_lcd_st7701"))
	guitionLib.AddDependency(respressif_componentspkg.GetLibrary("library_esp_lcd_panel_io_additions"))
	guitionLib.AddDependency(respressif_componentspkg.GetLibrary("library_esp_lcd_touch_gt911"))
	guitionLib.AddDependencies(corepkg.GetMainLib())

	// Example applications
	wcsExample := denv.SetupCppAppProjectForArduinoEsp32(mainpkg, "wcs_example", "wcs_example")
	wcsExample.AddDependency(wcsLib)

	guitionExample := denv.SetupCppAppProjectForArduinoEsp32(mainpkg, "guition_example", "guition_example")
	guitionExample.AddDependency(guitionLib)

	// Add example applications to the main package
	mainpkg.AddMainApp(wcsExample)
	mainpkg.AddMainApp(guitionExample)
	// Add libraries to the main package
	mainpkg.AddLibrary(dwoLib)
	mainpkg.AddLibrary(wcsLib)
	mainpkg.AddLibrary(guitionLib)

	return mainpkg
}
