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

The current source cleanup changed multiple upstream files. Before publishing
a redistributed XINU source release, the changed-file set must be reconciled
against the imported baseline and each affected file must receive a
syntax-appropriate APSL Exhibit A notice and an accurate modification date.
Do not invent an earlier date or overwrite an existing copyright notice.

## Change log

| Date | Change |
|---|---|
| 2026-11-24 | Added OSSD XINU provenance, licensing, and hardware-support documentation. |
