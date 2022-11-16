# Pull requests
All contributions must be offered as GitHub pull requests. Please read Github documentation for more info how to fork projects and create pull requests.

# Commit guidelines

All submitted changes have to pass our code review system. No code can be merged into the repository, without passing the review of another developer. In order to enable fast reviews, there are some requirements to be met. PRs that do not obey these rules, will not be considered during review and have no chance of being merged into the code base. We try to acknowledge that everyone has their own style, but we also try to keep things consistent across a large codebase. We strive to maintain friendly and helpful language when reviewing, give concrete suggestions how things could be done instead, in favor of just disliking a piece of code. As soon as a PR is created, it will be looked at by a reviewer. If you want to signal that it's being worked on/changed, put a WIP in the name, or add a WIP label. After a review round, address the comments of the review, and wait for the reviewer to mark them as 'resolved'. Once everything is resolved, the PR will enter the integration process.

1) Review requirements
	-	The code works
	-	PR branch has to be based on the latest released branch HEAD version (unless the requested change is a hotfix for existing release)
	-	Unit tests are included - at least one test closely related to the change (even for proof-of-concept code)
	-	There are no code style or formatting violations (-> 5)
	-	Good PR description (-> 2)

2) Meaningful PR description is required
	-	What does it do? Which were the considerations while implementing it, if relevant? Did you consider other options, but decided against them for a good reason?
	-	If reviewer does not understand the content from the description, review will be denied
	- 	PR description matches exactly the code change
	
3) Target is exactly one issue
	-	It changes exactly one issue in the code
	-	No mixing of refactoring and other work
	-	e.g. NOT: adds function to API and fixed memory leak

4) Should be small
	-	Typically only a few lines
	-	Cannot be split
	-	As long as the reviewer can see a pattern the commit can be longer (e.g. renaming schema)

5) Code style
	-	The coding style guidelines are fixed and commented in the [.clang-tidy](.clang-tidy) and [.clang-format](.clang-format) files in the repository root
	-	Those files will be picked up by Visual Studio and ReSharper++ (under some circumstances, the actual way how to do it seems more complicated than anticipated, see below)
	-	[More detailed Coding Style Guidelines in RaCO OSS](#more-detailed-coding-style-guidelines-in-raco-oss)

6) It is self-contained
	-	It doesn't break anything

7) Works for all platforms
	-	Make sure the unit tests succeed on the supported Windows and Linux platforms
	-	PRs that don't pass build servers will not be reviewed
	
8) Respects licenses and external IP
	-	PRs must not contain any copied code
	-	PR content is free of external IP
	-	All mandatory (open source) license restrictions are satisfied
	
9) Bundle commits into multiple PRs when it makes sense
	-	If you can split a complex feature into smaller PRs where some of them can be integrated earlier than others, do so
	-	When submitting multiple PRs which depend on each other, please mark their dependency by adding a line like this: "Depends-On: <change-url>" where <change-url> is a link to another PR which needs to go in first.


## More Detailed Coding Style Guidelines in RaCO OSS
### Points not checked / described by clang-tidy or clang-format
* Minimize the number of include files in a header file:
	-	Use forward declarations whenever possible in a few lines
	-	As a rule of thumb, includes should only exist for base classes and classes used by value. They are not necessary for references or pointers
* For include files, use quotes if the included file is in the same solution. Use angular brackets only for files which are not in the solution or for files which are part of an external library (= the file is in the "third-party" folder - e. g. Ramses or boost)
* For lambda functions, avoid using implicit captures with '[&]' and '[=]'
	-	(one) reason: implicit captures frequently can hide life-time issues with objects - e. g. a captured "this" pointer or shared_ptr binds the life-time of the lambda to the life-time of the captured pointer, and it makes potential issues more obvious if the capture is done explicitly
* Do not use "catch(...)" ever. It will capture crashes (access violation etc) in Windows, leaving the application in an unknown state.
* Only use "catch" for specific exceptions - do not do general "catch everything which happens in here" constructs
	-	"catch" should only be used if it is known which exceptions should be captured, and then it should be done as narrowly as possible
	-	In general it is better to have a crash due an uncaught exception on unexpected behaviour than an undefined state of the data (which is what happens if an unexpected exception is thrown at an unexpected place and it is unclear if the data is left in a consistent state)
### External Libraries
* In general prefer using mature and maintained external libraries to writing something yourself
* If a functionality is available in a C++ standard library, use that version rather than an external library
* Do not just pull in additional external libraries
* If you need an additional external library, first check what is already available (make sure to check what Ramses already uses in the ramses/external folder). If that is not sufficient and you know a library which would help, check with the development team if there are any objections against using a new external library.
* Make sure to check the license conditions of the external library. Virulent licenses like GPL are a no-go.
* Any third party libraries are added as submodules to the third_party folder
	-	Ideally use directly a repository as it is released by the third_party vendor, if that is not possible (e. g. because the code needs to be patched), create a separate repository on our Bitbucket and use that as a submodule