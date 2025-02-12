#!groovy

def jobMatrix(String type, List specs) {
  def nodes = [:]
  for (spec in specs) {
    def job = ""
    def selector = ""
    def os = ""
    def ver = ""

    if (type == 'build') {
      job = "${spec.os}-${spec.ver}-${spec.arch}-${spec.compiler}"
      selector = "${spec.os}-${spec.ver}-${spec.arch}"
      os = spec.os
      ver = spec.ver
    } else { // == 'check'
      job = "${spec.name}"
      selector = 'fedora-36-x86_64'
      os = 'fedora'
      ver = '36'
    }

    def label = "${job}"
    def extra = spec.extra

    nodes[label] = {
      node(selector) {
        githubNotify(context: "${label}", description: 'Building ...', status: 'PENDING')
        try {
          deleteDir()
          checkout scm

          def jobscript = 'job.sh'
          def ctestcmd = "ctest -S ODCTest.cmake -VV --output-on-failure"
          sh "echo \"set -e\" >> ${jobscript}"
          sh "echo \"export LABEL=\\\"\${JOB_BASE_NAME} ${label}\\\"\" >> ${jobscript}"
          if (selector =~ /alice/) {
            ctestcmd = "aliBuild build --defaults o2 ODC --debug && cd ODC && alienv setenv ODC/latest,CMake/latest -c ctest -VV -S ODCTest.cmake -DTEST_ONLY=ON -DCTEST_BINARY_DIRECTORY=\\\\\\\${ALIBUILD_WORK_DIR}/BUILD/ODC-latest/ODC"
          }
          def containercmd = "singularity exec -B/shared ${env.SINGULARITY_CONTAINER_ROOT}/odc/${os}.${ver}.sif bash -l -c \\\"${ctestcmd} ${extra}\\\""
          if (selector =~ /alice/) {
            containercmd = "singularity exec -f --no-home -B.:/root/ODC -w ${env.SINGULARITY_CONTAINER_ROOT}/odc/${os}.${ver}.sif bash -l -c \\\"${ctestcmd} ${extra}\\\""
          }
          sh """\
            echo \"echo \\\"*** Job started at .......: \\\$(date -R)\\\"\" >> ${jobscript}
            echo \"echo \\\"*** Job ID ...............: \\\${SLURM_JOB_ID}\\\"\" >> ${jobscript}
            echo \"echo \\\"*** Compute node .........: \\\$(hostname -f)\\\"\" >> ${jobscript}
            echo \"unset http_proxy\" >> ${jobscript}
            echo \"unset HTTP_PROXY\" >> ${jobscript}
            echo \"${containercmd}\" >> ${jobscript}
          """
          sh "cat ${jobscript}"
          sh "utils/ci/slurm-submit.sh \"ODC \${JOB_BASE_NAME} ${label}\" ${jobscript}"

          deleteDir()
          githubNotify(context: "${label}", description: 'Success', status: 'SUCCESS')
        } catch (e) {
          // def tarball = "${type}_${job}_dds_logs.tar.gz"
          // if (fileExists("build/test/.DDS")) {
            // sh "tar czvf ${tarball} -C \${WORKSPACE}/build/test .DDS/"
            // archiveArtifacts tarball
          // }

          deleteDir()
          githubNotify(context: "${label}", description: 'Error', status: 'ERROR')
          throw e
        }
      }
    }
  }
  return nodes
}

pipeline{
  agent none
  options { ansiColor('xterm') }
  stages {
    stage("CI") {
      steps{
        script {
          def builds = jobMatrix('build', [
            [os: 'fedora', ver: '36',    arch: 'x86_64', compiler: 'gcc-12'],
            // [os: 'centos', ver: '8stream.alice', arch: 'x86_64', compiler: 'gcc-10'],
          ])

          def checks = jobMatrix('check', [
            [name: 'sanitizers', extra: '-DENABLE_SANITIZERS=ON'],
          ])

          parallel(builds + checks)
        }
      }
    }
  }
}
