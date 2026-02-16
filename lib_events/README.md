# lib_events
This is a software events library that can be used by any Zephyr application
It can be used to register a callback to an event, report happening of an event and get notified of an event via a callback

Important functions:

- `lib_events_init`	- this must be called before calling the other functions
- `lib_events_callback_add`
- `lib_events_callback_remove`
- `lib_events_report_event`

Typically an application would add this library in its `lib` directory
Below is an example of an application directory structure that can be followed

```
  app
  |-- boards
  |-- dts
  |-- modules
  |-- src
	    |-- app
	    |-- bsp
	    |-- include
	    |-- lib
	  	  	  |-- lib_events
```

To add this library as a git submodule use the below git command:

`git submodule add https://github.com/acmecpu/lib_events.git`

OR

`git submodule add git@github.com:acmecpu/lib_events.git`