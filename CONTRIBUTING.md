# Developer Notes for Contributing Code

We welcome and appreciate any kind of use case evaluations, feature requests,
bug reports and code contributions. To simplify cooperation, we follow the
guidelines described in the following.

## Developer Notes

### Versioning

We follow the [semantic versioning](https://semver.org/) stategy. In short, the version numbers denote the following:

```
<major>.<minor>.<patch>
   |       |       |------ backwards-compatible (no new feature)
   |       |-------------- backwards-compatible (new features or deprecations)
   |---------------------- breaking change
```

### Issues

Before opening an issue, check if the following conditions are met:

- The issue is related to DRace (not DynamoRIO). To verify that, execute your application using `drrun.exe` without any client.
- There is no similar issue

To make the investigation as simple as possible, please include the following information in your ticket:

- DRace version: obtain it by running DRace with the `--version` flag
- DynamoRIO version: run ` drrun -version`

If possible, try to run DynamoRIO + DRace + <App> in Windbg and provide callstacks.

### Code Contributions

Since version 1.0.4 we follow the [Conventional Changelog](https://www.conventionalcommits.org/en/) strategy.
Please write your commit message according to this standard.

## Doxygen Style

We use the
[`JAVADOC_AUTOBRIEF` Doxygen style](http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html).

This example should illustrate all you need:

```c++
/**
 * Multi-line comment opened with double-star "/**".
 * First paragraph is brief description.
 *
 * Paragraphs are separated by empty comment lines. This is
 * the second paragraph and is not included in the brief
 * description.
 *
 * \todo
 * ToDo paragraphs will be rendered in a highlighted box and are
 * also summarized on a single ToDo list page. Very useful!
 *
 * Return values are documented like this:
 * 
 * \returns   true   if the specified number of wombats has
 *                   been successfully wombatified with the given
 *                   factor
 *            false  otherwise
 *
 * To add references to otherwise (non-concept) related interface
 * definitions, use:
 *
 * \see drace::foo
 */
bool foo(
   /// Single-line comments opened with *three* forward-slashes.
   /// This is a convenient way to describe function parameters.
   /// This style makes undocumented parameters easy to spot.
   int     num_foos,
   double  foo_factor  ///< Postfix-style with "<", comment placed *after* described parameter
) {
  // Any comment here is ignored, no matter how it is formatted.
}
```
