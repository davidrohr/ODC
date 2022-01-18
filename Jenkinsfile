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
      selector = 'fedora-34-x86_64'
      os = 'fedora'
      ver = '34'
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
          def ctestcmd = "ctest ${extra} -S ODCTest.cmake -VV --output-on-failure"
          sh "echo \"set -e\" >> ${jobscript}"
          sh "echo \"export LABEL=\\\"\${JOB_BASE_NAME} ${label}\\\"\" >> ${jobscript}"
          if (selector =~ /^macos/) {
            sh """\
              echo \"export DDS_ROOT=\\\"\\\$(brew --prefix dds)\\\"\" >> ${jobscript}
              echo \"export PATH=\\\"\\\$(brew --prefix dds)/bin:\\\$PATH\\\"\" >> ${jobscript}
              echo \"${ctestcmd}\" >> ${jobscript}
            """
            sh "cat ${jobscript}"
            sh "bash ${jobscript}"
          } else {
            def containercmd = "singularity exec -B/shared ${env.SINGULARITY_CONTAINER_ROOT}/odc/${os}.${ver}.sif bash -l -c \\\"${ctestcmd} ${extra}\\\""
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
          }

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
  stages {
    stage("CI") {
      steps{
        script {
          def builds = jobMatrix('build', [
            [os: 'fedora', ver: '34',    arch: 'x86_64', compiler: 'gcc-11'],
            [os: 'macos',  ver: '10.15', arch: 'x86_64', compiler: 'apple-clang-12'],
            [os: 'macos',  ver: '11',    arch: 'x86_64', compiler: 'apple-clang-13'],
            [os: 'macos',  ver: '12',    arch: 'arm64', compiler: 'apple-clang-13'],
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
