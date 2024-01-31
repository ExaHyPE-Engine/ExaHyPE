<!-- omit in toc -->
# Contributing to Peano

All types of contributions are encouraged and valued. See the [Table of Contents](#table-of-contents) for different ways to help and details about how this project handles them. Please make sure to read the relevant section before making your contribution. It will make it a lot easier for us maintainers and smooth out the experience for all involved. The community looks forward to your contributions.

> And if you like the project, but just don't have time to contribute, that's fine. There are other easy ways to support the project and show your appreciation, which we would also be very happy about:
> - Star the project
> - Tweet about it
> - Refer this project in your project's readme
> - Mention the project at local meetups and tell your friends/colleagues

<!-- omit in toc -->
## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Join The Project Team](#join-the-project-team)
- [I Have a Question](#i-have-a-question)
- [I Want To Contribute](#i-want-to-contribute)
  - [Reporting Bugs](#reporting-bugs)
  - [Suggesting Enhancements](#suggesting-enhancements)
  - [Improving The Documentation](#improving-the-documentation)
- [Styleguides](#styleguides)


## Code of Conduct

This project and everyone participating in it is governed by the
[Code of Conduct](./CODE_OF_CONDUCT.md).
By participating, you are expected to uphold this code. Please report unacceptable behavior
to tobias.weinzierl (at) durham.ac.uk.


## Join The Project Team

We use Slack for internal communication. If you want join our workspace, write an email to tobias.weinzierl (at) durham.ac.uk.


## I Have a Question

> If you want to ask a question, we assume that you have read the available [Documentation](https://hpcsoftware.pages.gitlab.lrz.de/Peano/html/index.html).

Before you ask a question, it is best to search our Slack channels first. It is also advisable to search the internet for answers first.

If you then still feel the need to ask a question and need clarification, we recommend the following:

- Decide into which Slack channel your question fits.
- Provide as much context as you can about what you're running into.
- Provide project and platform versions, depending on what seems relevant.

We will then take care of your question as soon as possible.


## I Want To Contribute

> ### Legal Notice <!-- omit in toc -->
> When contributing to this project, you must agree that you have authored 100% of the content, that you have the necessary rights to the content and that the content you contribute may be provided under the project license.
> When using the software please make sure to cite it. Details can be found under [www.peano-framework.org](https://tobiasweinzierl.webspace.durham.ac.uk/software/peano/).

### Reporting Bugs

<!-- omit in toc -->
#### Before Submitting a Bug Report

A good bug report shouldn't leave others needing to chase you up for more information. Therefore, we ask you to investigate carefully, collect information and describe the bug in detail in your report. Please complete the following steps in advance to help us fix any potential bug as fast as possible.

- Make sure that you are using the latest version.
- Determine if your bug is really a bug and not an error on your side e.g., using incompatible environment components/versions (Make sure that you have read the [documentation](https://hpcsoftware.pages.gitlab.lrz.de/Peano/html/index.html). If you are looking for support, you might want to check [this section](#i-have-a-question)).
- To see if other users have experienced (and potentially already solved) the same bug you are having, check if there is not already a bug report existing in our Slack channels.
- Also make sure to search the internet (including Stack Overflow) to see if users outside of the project have discussed the bug.
- Collect information about the bug:
  - Stack trace (Traceback)
  - OS, Platform and Version (Windows, Linux, macOS, x86, ARM)
  - Version of the interpreter, compiler, SDK, runtime environment, package manager, depending on what seems relevant.
  - Possibly your input and the output
  - Can you reliably reproduce the bug? And can you also reproduce it with older versions?

<!-- omit in toc -->
#### How Do I Submit a Good Bug Report?

> You must never report security related issues, vulnerabilities or bugs including sensitive information in public. Instead sensitive bugs must be sent by email to tobias.weinzierl (at) durham.ac.uk.

We use Slack to report bugs and errors. If you run into an issue with the project:

- Create a bug report in one of our Slack channels.
- Explain the behavior you would expect and the actual behavior.
- Please provide as much context as possible and describe the *reproduction steps* that someone else can follow to recreate the bug on their own. This usually includes your code. For good bug reports you should isolate the problem and create a reduced test case.
- Provide the information you collected in the previous section.

Once it's filed:

- A team member will try to reproduce the bug with your provided steps. If there are no reproduction steps or no obvious way to reproduce the bug, the team will ask you for those steps.
- If the team is able to reproduce the bug, it will be left to be implemented.


### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for Peano, **including completely new features and minor improvements to existing functionality**. Following these guidelines will help maintainers and the community to understand your suggestion and find related suggestions.

<!-- omit in toc -->
#### Before Submitting an Enhancement

- Make sure that you are using the latest version.
- Read the [documentation](https://hpcsoftware.pages.gitlab.lrz.de/Peano/html/index.html) carefully and find out if the functionality is already covered, maybe by an individual configuration.
- Perform a search in our Slack channels to see if the enhancement has already been suggested. If it has, add a comment to the existing thread instead of opening a new one.
- Find out whether your idea fits with the scope and aims of the project. It's up to you to make a strong case to convince the project's developers of the merits of this feature. Keep in mind that we want features that will be useful to the majority of our users and not just a small subset. If you're just targeting a minority of users, consider writing an add-on/plugin library.

<!-- omit in toc -->
#### How Do I Submit a Good Enhancement Suggestion?

Enhancement suggestions are made in Slack.

- Provide a **step-by-step description of the suggested enhancement** in as many details as possible.
- **Describe the current behavior** and **explain which behavior you expected to see instead** and why. At this point you can also tell which alternatives do not work for you.
- You may want to **include screenshots and animated GIFs** which help you demonstrate the steps or point out the part which the suggestion is related to.
- **Explain why this enhancement would be useful** to most Peano users. You may also want to point out the other projects that solved it better and which could serve as inspiration.

### Improving The Documentation

We use Doxygen to generate our [documentation](./documentation/).
Make sure to contribute your changes to the documentation.

## Styleguides

Format your changes by using

```
python3 format.py
```
