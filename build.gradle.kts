import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    alias(libs.plugins.androidLibrary)
    alias(libs.plugins.kotlinAndroid)
    `maven-publish`
}

android {
    namespace = "io.github.tobelogin"
    compileSdk = 36

    defaultConfig {
        minSdk = 24

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        consumerProguardFiles("consumer-rules.pro")
        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON",
                )
                cppFlags("-std=c++17")
            }
        }

        aarMetadata {
            minCompileSdk = android.defaultConfig.minSdk
        }

        publishing {
            singleVariant("release") {
                withSourcesJar()
            }
        }
    }

    buildTypes {
        debug {
            isMinifyEnabled = false // 为 true 时编译器认为一个类没有被使用就会把它优化掉
        }

        release {
            isMinifyEnabled = false
//            proguardFiles(
//                getDefaultProguardFile("proguard-android-optimize.txt"),
//                "proguard-rules.pro"
//            )
        }
    }
    externalNativeBuild {
        cmake {
            path("src/main/cpp/CMakeLists.txt")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }
    kotlin {
        compilerOptions {
            jvmTarget = JvmTarget.fromTarget("21")
        }
    }
}

publishing {
    publications {
        register<MavenPublication>("release") {
            groupId = "io.github.tobelogin"
            artifactId = "native-list2webp"
            version = "0.1"

            pom {
                name = "native-list2webp"
                description = "Native way to convert list to webp for Android"
                url = "https://github.com/tobelogin/native-list2webp"
                licenses {
                    license {
                        name = "LGPL version 2.1 or later"
                        url = "https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html"
                    }
                }
                developers {
                    developer {
                        id = "zjab"
                        name = "zjab"
                        email = "108188072+tobelogin@users.noreply.github.com"
                    }
                }
                scm {
                    url = "https://github.com/tobelogin"
                }
            }

            afterEvaluate {
                from(components["release"])
            }
        }
    }
    repositories {
        maven {
            name = "buildRepo"
            val releasesRepoUrl = layout.buildDirectory.dir("repos/releases")
            val snapshotsRepoUrl = layout.buildDirectory.dir("repos/snapshots")
            url = uri(if (version.toString().endsWith("SNAPSHOT")) snapshotsRepoUrl else releasesRepoUrl)
        }
    }
}