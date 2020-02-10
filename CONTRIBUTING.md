# Developer Notes for Contributing Code

We welcome and appreciate any kind of use case evaluations, feature requests,
bug reports and code contributions. To simplify cooperation, we follow the
guidelines described in the following.

## Developer Notes

### Versioning

We follow the [semantic versioning](https://semver.org/) strategy. In short, the version numbers denote the following:

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

If possible, try to run DynamoRIO + DRace + <App> in WinDbg and provide callstacks.

### Code Contributions

Since version 1.0.4 we follow the [Conventional Changelog](https://www.conventionalcommits.org/en/) strategy.
Please write your commit message according to this standard.

## Code Style

To ensure a good readability of our codebase, we follow and enforce style guides.

### Source Code

We use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to ensure a consistent formatting of our codebase.
To this, we provide both a `.editorconfig` and a `.clang-format` file to format the code according to the project's rules.
The `.editorconfig` is just for convenience to setup the editor similar to the formatting rules.
This includes (among others) configuration for line endings, tabs and spaces.

Please make sure that you format the source code using clang-format and the provided `.clang-format` rule file.
If you use Visual Studio Code, an option is to use the clang-format extension and set `"editor.formatOnSave": true`.
In addition, we provide a bash script `contrib/format-files.sh` to format the complete code-base.
This can be executed in a MinGW terminal, or in the git bash.
Unfortunately, we cannot accept code contributions which do comply with these rules.

### Doxygen Style

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

## Unit Tests

When fixing a bug, please add a unit test that fails until the bug is resolved.
When adding features, a unit test is mandatory.

## Static Code Analysis

We use [cppcheck](http://cppcheck.sourceforge.net/) to statically analyze the codebase.
This analysis is automatically performed in the CI where we require a error-free report to pass.
However, we do not enforce the developer to follow all rule-recommendations.
If there is some good reason to decide against a rule, annotate this by either adding a cppcheck inline annotation, or adding a suppression in `contrib/suppressions.txt`.

To run cppcheck locally, execute this command in the build directory:

```
cppcheck.exe --project=compile_commands.json --enable=all --inline-suppr --suppressions-list=suppressions.txt --quiet --error-exitcode=1
```

A note regarding missing headers is ok and is not treated as an error.

## Add External Dependency

Please follow the following steps when adding a new dependency to an external library.
Please also check carefully, that this dependency is really needed.

1. discuss with DRace team, check license
2. add as library as a git submodule in vendor, similar to existing ones
3. add cmake library target, similar to the existing ones in `vendor/CMakeLists.txt`
4. make sure license copy works
5. document external dependency in `DEPENDENCIES.md`
