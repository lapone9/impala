# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

# The dependency on shaded-deps is only added to avoid parallel downloads
# of dependencies. For more details see IMPALA-7051, which was a similar issue.
add_custom_target(ext-data-source ALL DEPENDS gen-deps impala-parent
    shaded-deps-hive-exec shaded-deps-s3a-aws-sdk
  COMMAND $ENV{IMPALA_HOME}/bin/mvn-quiet.sh -B install -DskipTests
)
