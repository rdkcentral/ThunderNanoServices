# Wayland Compositor Library

This library provides Wayland compositor functionality for ThunderNanoServices. It is designed to integrate seamlessly with the Thunder framework and enable efficient rendering and display management.


## Usage
To properly start Thunder with a Westeros compositor, use the following command:

```shell
XDG_RUNTIME_DIR=/run LD_PRELOAD=libwesteros_gl.so WESTEROS_DRM_CARD=/dev/dri/card1 WESTEROS_GL_NO_VBLANK=1 WAYLAND_DISPLAY=wayland-0 Thunder -F
```

While the compositor configures most of the required environment variables for its operation, these variables are not automatically propagated to the plugins. 
Ensure that any necessary adjustments are made accordingly.

## Contributing

Contributions are welcome! Please follow the project's coding standards and submit pull requests for review.

## License

This project is licensed under the [MIT License](../../../LICENSE).

## Contact

For questions or support, please contact the maintainers through the project's repository.