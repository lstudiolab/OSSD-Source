# XINU modification record

## Imported baseline

- Project: OSSD / XINU
- Upstream source repository: https://github.com/lstudiolab/darwin-xnu
- Imported baseline in OSSD-Source: `5b4847b4d910bf78733454c9f4d161f062d953d1`
- Current source branch: `main`

## OSSD changes recorded

- Imported the XNU source tree into `XINU/`.
- Performed source-comment cleanup on supported source formats. Legal,
  copyright, SPDX, and attribution notices were intended to be retained;
  nevertheless, each file must be checked against its upstream license
  obligations before redistribution.
- Added OSSD project documentation and hardware-support planning.
- Future kernel changes must be recorded here and carry the file-level
  notices required by the applicable upstream license.

## File-level APSL notice requirement

APSL 2.0 Section 2.2(b) requires the Exhibit A notice to be present in each
source file containing a Modification and requires modified files to carry
prominent notices stating that the file was changed and the date of change.
A directory-level notice is not a substitute for those file-level notices.

On 2026-11-24, the repository automation reconciled the source tree against
the imported baseline and added the dated OSSD modification notice to 2,292
changed files containing one of the recognized APSL 2.0 file-header forms.
The script supports both the standard Apple license header and the Apple OS
Reference license header.

A further 584 changed source files did not contain either recognized APSL
header form and were deliberately not relabeled as APSL-covered. Their
upstream license and provenance must be audited individually before a
redistributed release. A missing APSL marker is not proof that a file is
unlicensed or that APSL applies to it.

The notice pass does not replace a full component-by-component license audit.
Do not overwrite original copyright notices or apply APSL notices to code
governed by a different license.

## Change log

| Date | Change |
|---|---|
| 2026-11-24 | Added OSSD XINU provenance, licensing, and hardware-support documentation. |
| 2026-11-24 | Added dated OSSD modification notices to 2,292 changed APSL-header source files; 584 files without recognized APSL headers remain for separate license audit. |
