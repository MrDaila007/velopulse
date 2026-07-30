allprojects {
    repositories {
        google()
        mavenCentral()
    }
}

val newBuildDir: Directory =
    rootProject.layout.buildDirectory
        .dir("../../build")
        .get()
rootProject.layout.buildDirectory.value(newBuildDir)

subprojects {
    val newSubprojectBuildDir: Directory = newBuildDir.dir(project.name)
    project.layout.buildDirectory.value(newSubprojectBuildDir)
}
subprojects {
    project.evaluationDependsOn(":app")
}

tasks.register<Delete>("clean") {
    delete(rootProject.layout.buildDirectory)
}

// reactive_ble_mobile 5.5.0 declares compileSdk 33, while its current AndroidX
// dependencies require 34+. Keep the plugin isolated and compile every Android
// library against the app's pinned API without modifying the pub cache.
subprojects {
    if (name == "reactive_ble_mobile") {
        afterEvaluate {
            extensions.findByType<com.android.build.api.dsl.LibraryExtension>()?.apply {
                compileSdk = 36
            }
        }
    }
}
