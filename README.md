# Car-Demo

A simple scene with a high-poly sports car model in a small garage. 

I made it over the weekend for a job application. Everything was written from scratch in C++ 17.

![screenshot 3](/screenshots/screenshot-3.png)

## Features

- Lighting
- Real-time point shadow mapping
- Real-time environment mapping
- Soft shadows
- 1'500'000 triangles
- Transparency
- Text rendering

![screenshot 2](/screenshots/screenshot-2.png)

## Try it out

Download the latest release from [here](https://github.com/blat-blatnik/Car-Demo/releases) and run the executable.

Note that you will need a _moderately_-powerful GPU. For reference, I get 110 FPS on a mobile GTX 1050, and 30 FPS on an Intel HD 630. Your GPU will also need to support at least OpenGL 4.3.

## How to compile ..

### .. with MSVC

A zipped Visual Studio 2020 solution is provided in the [`bin/`](/bin/) directory. Everything should already be set up.

### .. with G++

Make sure you've installed the [GLFW](https://www.glfw.org/download.html) library. Static libraries of GLFW for Windows, Mac, and Linux are included in the [`lib/`](lib/) directory of this project in case you can't install GLFW. 

Navigate to the root of this project and compile the project with this line:

```bash
$ g++ -std=c++17 source/*.cpp -lm -lglfw3
```

## TODO

This was a very quick project, so there is a lot more that I could add. 

- [x] fix gamma correction on Intel drivers
- [ ] finish stage lights
  - [ ] attach actual lighting to the models
  - [ ] shadow map the spotlights
  - [ ] animate the spotlights
- [ ] volumetric shadows
  - [ ] add smoke (or fog) to the room
  - [ ] animate the smoke
- [ ] make the car driveable
  - [ ] animate car wheels
  - [ ] simple physics
- [ ] find better looking textures for the garage
- [ ] find a better car model
- [ ] fix shadow acne

I think this whole scene would look especially good with some nice volumetric shadows (and some better textures for the background).

## Libraries

- [GLFW](https://www.glfw.org/) for cross platform window opening and OpenGL context creation.
- [GLAD](https://glad.dav1d.de/) for loading OpenGL function calls.
- [stb_image](https://github.com/nothings/stb) for loading textures.
- [stb_truetype](https://github.com/nothings/stb) for loading fonts.
- [tinyobj](https://github.com/tinyobjloader/tinyobjloader) for loading `.obj` files so I could convert them to my own `.model` file format.


## Credits

- James May ([kimzauto](https://free3d.com/user/kimzauto)) for the [Bugatti Chiron car model](https://free3d.com/3d-model/bugatti-chiron-2017-model-31847.html).
- Haider ([iphacker786](https://free3d.com/user/iphacker786)) for the [stage light model](https://free3d.com/3d-model/spotlights-84030.html).
- The textures for the [concrete](https://cc0textures.com/view?id=Concrete015) floor and [brick](https://cc0textures.com/view?id=Bricks042) walls of the garage were downloaded from [CC0 Textures](https://cc0textures.com/).
