<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# libRamsesAdaptor

## ramses_adaptor

### Dispatcher strategy

We need a well defined dispatching strategy of changes recordered in the DataChangeRecorder.

#### Requirements:
* Our data model is always in a consistent state when dispatch is called.
  * no ValueHandle reference to a EditorObject which is recorded as deleted.
* Recorded order of changes should never matter.

#### Naive Sketch:
1. Create all adaptors for recorded created EditorObject's
2. Reparent all adaptors according to the children changes in EditorObject's
3. Change all remaining ValueHandle change.
4. Delete all adaptors for all deleted EditorObject's (order of deletion is important)
   1. Delete MeshNode's before Mesh's and Material's
   2. Delete Logic elements before Node's
